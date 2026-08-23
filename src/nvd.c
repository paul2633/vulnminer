#include <curl/curl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <yyjson.h>

#include "config.h"
#include "display.h"
#include "http.h"
#include "nvd.h"
#include "utils.h"

#define NVD_API "https://services.nvd.nist.gov/rest/json/cves/2.0"
#define RESULTS_PER_PAGE 50
#define DAYS_PER_WINDOW 120

#define DAYS(n) ((time_t)(n) * 24 * 60 * 60)
#define MAX_TIMEOUT 16

static void nvd_reset_client(http_client_t *client, config_t *config) {
    http_client_destroy(client);
    http_client_init(client);

    if (config->nvd_api_key != NULL) {
        char *key = NULL;
        EXIT_IF(asprintf(&key, "apiKey: %s", config->nvd_api_key) == -1, "asprintf");
        http_client_add_header(client, key);
        free(key);
    }
}

static yyjson_doc *nvd_get(config_t *config, http_client_t *client, const char *url) {
    size_t delay = 0;

    for (;;) {
        sleep(delay);

        http_response_t response = {0};
        long status = 0;

        CURLcode err = http_get(client, url, &response, &status);

        if (err == CURLE_OPERATION_TIMEDOUT) {
            delay = 0;
            nvd_reset_client(client, config);
        }

        else if (err != CURLE_OK) {
            EXIT_IF(true, curl_easy_strerror(err));
        }

        else if (status == 429) {
            delay = delay == 0 ? 1 : delay * 2 > MAX_TIMEOUT ? delay : delay * 2;
            nvd_reset_client(client, config);
        }

        else if (status < 200 || status >= 300) {
            EXIT_IF(true, "unexpected HTTP status %ld", status);
        }

        else {
            delay /= 2;

            yyjson_doc *doc = yyjson_read(response.data, response.size, 0);
            free(response.data);
            EXIT_IF(doc == NULL, "yyjson_read");

            return doc;
        }

        free(response.data);
    }
}

static int nvd_probe_window(config_t *config, http_client_t *client, const char *window_end_url, const char *window_start_url, unsigned cwe_id) {

    char *url = NULL;
    EXIT_IF(asprintf(&url, "%s?resultsPerPage=0&startIndex=0&pubEndDate=%s&pubStartDate=%s&cweId=CWE-%u", NVD_API, window_end_url, window_start_url, cwe_id) ==
                -1,
            "asprintf");

    yyjson_doc *doc = nvd_get(config, client, url);

    free(url);

    yyjson_val *root = yyjson_doc_get_root(doc);
    EXIT_IF(root == NULL, "yyjson_doc_get_root");

    yyjson_val *total = yyjson_obj_get(root, "totalResults");
    EXIT_IF(total == NULL || !yyjson_is_uint(total), "totalResults");

    int result = yyjson_get_int(total);

    yyjson_doc_free(doc);
    return result;
}

static void parse_references(yyjson_doc *doc) {

    yyjson_val *root = yyjson_doc_get_root(doc);
    EXIT_IF(root == NULL, "yyjson_doc_get_root");

    yyjson_val *vulns = yyjson_obj_get(root, "vulnerabilities");
    EXIT_IF(vulns == NULL || !yyjson_is_arr(vulns), "vulnerabilities");

    size_t count = yyjson_arr_size(vulns);

    for (size_t i = count; i-- > 0;) {

        yyjson_val *vuln = yyjson_arr_get(vulns, i);

        yyjson_val *cve = yyjson_obj_get(vuln, "cve");
        EXIT_IF(cve == NULL || !yyjson_is_obj(cve), "cve");

        yyjson_val *refs = yyjson_obj_get(cve, "references");
        if (refs == NULL || !yyjson_is_arr(refs))
            continue;

        yyjson_val *ref;
        size_t j, max_refs;

        yyjson_arr_foreach(refs, j, max_refs, ref) {
            yyjson_val *url = yyjson_obj_get(ref, "url");

            if (url != NULL && yyjson_is_str(url)) {

                yyjson_val *tags = yyjson_obj_get(ref, "tags");

                bool is_patch = false;

                if (tags != NULL && yyjson_is_arr(tags)) {
                    yyjson_val *tag;
                    size_t i, max;

                    yyjson_arr_foreach(tags, i, max, tag) {
                        if (yyjson_is_str(tag) && strcmp(yyjson_get_str(tag), "Patch") == 0) {
                            is_patch = true;
                            break;
                        }
                    }
                }
                /*
                if (is_patch && strstr(yyjson_get_str(url), "github.com") != NULL && strstr(yyjson_get_str(url), "/commit/") != NULL) {

                    yyjson_val *published = yyjson_obj_get(cve, "published");

                    if (published != NULL && yyjson_is_str(published)) {
                        yyjson_val *id = yyjson_obj_get(cve, "id");
                        EXIT_IF(id == NULL || !yyjson_is_str(id), "id");

                        printf("%s %s %s\n", yyjson_get_str(id), yyjson_get_str(published), yyjson_get_str(url));
                    }
                }
                */
                // yyjson_val *published = yyjson_obj_get(cve, "published");
                // yyjson_val *id = yyjson_obj_get(cve, "id");
                // printf("%s %s %s\n", yyjson_get_str(id), yyjson_get_str(published), yyjson_get_str(url));
            }
        }
    }
}

static void nvd_download_window(config_t *config, http_client_t *client, display_t *display, time_t window_end, time_t window_start, unsigned cwe_id) {

    char window_end_url[32], window_start_url[32];
    date_to_url_format(window_end_url, sizeof(window_end_url), window_end);
    date_to_url_format(window_start_url, sizeof(window_start_url), window_start);

    display_update(display, cwe_id, 0, 0, window_start, window_end);
    int total_results = nvd_probe_window(config, client, window_end_url, window_start_url, cwe_id);
    int total_pages = (total_results + RESULTS_PER_PAGE - 1) / RESULTS_PER_PAGE;

    for (int i = total_pages - 1; i >= 0; i--) {

        char *url = NULL;
        EXIT_IF(asprintf(&url,
                         "%s?resultsPerPage=%d&startIndex=%d&pubEndDate=%s&pubStartDate=%s&cweId=CWE-%u",
                         NVD_API,
                         RESULTS_PER_PAGE,
                         i * RESULTS_PER_PAGE,
                         window_end_url,
                         window_start_url,
                         cwe_id) == -1,
                "asprintf");

        display_update(display, cwe_id, total_pages - i, total_pages, window_start, window_end);
        yyjson_doc *doc = nvd_get(config, client, url);

        free(url);
        parse_references(doc);
        yyjson_doc_free(doc);
    }
}

void nvd_request(config_t *config, display_t *display) {
    http_client_t client = {0};
    nvd_reset_client(&client, config);

    for (unsigned i = 0; i < config->cwe_ids_count; i++) {
        time_t window_end = config->cve_published_before;
        time_t window_start = window_end - DAYS(DAYS_PER_WINDOW) + 1;
        if (window_start < config->cve_published_after)
            window_start = config->cve_published_after;

        while (window_end > window_start) {
            nvd_download_window(config, &client, display, window_end, window_start, config->cwe_ids[i]);

            window_end -= DAYS(DAYS_PER_WINDOW);
            window_start -= DAYS(DAYS_PER_WINDOW);
            if (window_start < config->cve_published_after)
                window_start = config->cve_published_after;
        }
    }

    http_client_destroy(&client);
}
