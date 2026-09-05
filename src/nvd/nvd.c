#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <yyjson.h>
#include <curl/curl.h>
#include <pthread.h>

#include "config.h"
#include "history.h"
#include "http.h"
#include "jobs.h"
#include "nvd.h"
#include "nvd_parser.h"
#include "utils.h"

#define DAYS(n) ((time_t)(n) * 24 * 60 * 60)

#define NVD_API "https://services.nvd.nist.gov/rest/json/cves/2.0"
#define RESULTS_PER_PAGE 100
#define DAYS_PER_WINDOW 120
#define NVD_MAX_DELAY 16

static http_client_t *nvd_client_new(const char *api_key) {
    http_client_t *nvd_client = http_client_new();

    if (api_key != NULL) {
        char *line = NULL;
        EXIT_IF(asprintf(&line, "apiKey: %s", api_key) == -1, "asprintf");
        http_client_add_header(nvd_client, line);
        free(line);
    }

    return nvd_client;
}

static yyjson_doc *nvd_get(http_client_t *client, const char *url) {
    pthread_mutex_lock(&client->lock);

    while (true) {
        sleep(client->delay);

        http_response_t response = {0};
        long status = 0;

        CURLcode err = http_get(client, url, &response, &status);

        if (err == CURLE_OPERATION_TIMEDOUT) {
            http_client_reset(client);
            free(response.data);
            continue;
        }

        EXIT_IF(err != CURLE_OK, curl_easy_strerror(err));

        if (status == 429) {
            client->delay = client->delay == 0 ? 1 : client->delay * 2 > NVD_MAX_DELAY ? client->delay : client->delay * 2;
            http_client_reset(client);
            free(response.data);
            continue;
        }

        EXIT_IF(status < 200 || status >= 300, "HTTP error %ld", status);

        client->delay /= 2;
        pthread_mutex_unlock(&client->lock);

        yyjson_doc *doc = yyjson_read(response.data, response.size, 0);
        free(response.data);
        EXIT_IF(doc == NULL, "yyjson_read");

        return doc;
    }
}

static void date_to_str(char *dst, size_t size, time_t date, bool url_format) {
    struct tm tm = {0};
    EXIT_IF(gmtime_r(&date, &tm) == NULL, "gmtime_r");
    EXIT_IF(strftime(dst, size, url_format ? "%Y-%m-%dT%H:%M:%S.000Z" : "%d/%m/%Y", &tm) == 0, "strftime");
}

static char *parameters_to_url(int results_per_page, int start_index, time_t window_start, time_t window_end, unsigned cwe_id) {
    char start_url[sizeof("YYYY-MM-DDTHH:MM:SS.000Z")], end_url[sizeof("YYYY-MM-DDTHH:MM:SS.000Z")];
    date_to_str(start_url, sizeof(start_url), window_start, true);
    date_to_str(end_url, sizeof(end_url), window_end, true);

    char *url = NULL;
    EXIT_IF(asprintf(&url,
                     "%s?resultsPerPage=%d&startIndex=%d&pubStartDate=%s&pubEndDate=%s&cweId=CWE-%u",
                     NVD_API,
                     results_per_page,
                     start_index,
                     start_url,
                     end_url,
                     cwe_id) == -1,
            "asprintf");

    return url;
}

static void download_window(http_client_t *client, jobs_queue_t *github_queue, history_t *history, unsigned line_number, time_t window_start, time_t window_end,
                            unsigned cwe_id) {
    char start_display[sizeof("DD/MM/YYYY")], end_display[sizeof("DD/MM/YYYY")];
    date_to_str(start_display, sizeof(start_display), window_start, false);
    date_to_str(end_display, sizeof(end_display), window_end, false);

    char *suffix = NULL;
    EXIT_IF(asprintf(&suffix, "%s-%s   probing...", start_display, end_display) == -1, "asprintf");
    history_update_line(history, history->nvd_section, line_number, suffix, false);
    free(suffix);

    char *url = parameters_to_url(0, 0, window_start, window_end, cwe_id);
    yyjson_doc *doc = nvd_get(client, url);
    free(url);

    int total_results = nvd_parser_get_total_results(doc);
    yyjson_doc_free(doc);

    int total_pages = (total_results + RESULTS_PER_PAGE - 1) / RESULTS_PER_PAGE;

    for (int i = total_pages - 1; i >= 0; i--) {

        EXIT_IF(asprintf(&suffix, "%s-%s   fetching page %d/%d...", start_display, end_display, total_pages - i, total_pages) == -1, "asprintf");
        history_update_line(history, history->nvd_section, line_number, suffix, false);
        free(suffix);

        url = parameters_to_url(RESULTS_PER_PAGE, i * RESULTS_PER_PAGE, window_start, window_end, cwe_id);
        doc = nvd_get(client, url);
        free(url);

        nvd_parser_extract_commits(doc, github_queue, cwe_id, history);
        yyjson_doc_free(doc);
    }
}

void nvd_request(const config_t *config, jobs_queue_t *github_queue, history_t *history) {
    http_client_t *nvd_client = nvd_client_new(config->nvd_api_key);

    history_set_pending(history, history->nvd_section, config->cwe_ids_count);

    for (unsigned i = 0; i < config->cwe_ids_count; i++) {

        char *prefix = NULL;
        EXIT_IF(asprintf(&prefix, "CWE-%-3u", config->cwe_ids[i]) == -1, "asprintf");
        unsigned line_number = history_add_line(history, history->nvd_section, prefix, NULL);
        free(prefix);

        time_t window_end = config->cve_published_before;
        time_t window_start = window_end - DAYS(DAYS_PER_WINDOW) + 1;
        if (window_start < config->cve_published_after)
            window_start = config->cve_published_after;

        while (window_end > window_start) {

            download_window(nvd_client, github_queue, history, line_number, window_start, window_end, config->cwe_ids[i]);

            window_end -= DAYS(DAYS_PER_WINDOW);
            window_start -= DAYS(DAYS_PER_WINDOW);
            if (window_start < config->cve_published_after)
                window_start = config->cve_published_after;
        }

        history_update_line(history, history->nvd_section, line_number, "complete", true);
    }

    http_client_destroy(nvd_client);
}
