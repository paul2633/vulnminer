#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <yyjson.h>

#include "config.h"
#include "display.h"
#include "http.h"
#include "nvd.h"
#include "nvd_parser.h"
#include "utils.h"

#define NVD_API "https://services.nvd.nist.gov/rest/json/cves/2.0"
#define RESULTS_PER_PAGE 100
#define DAYS_PER_WINDOW 120

#define DAYS(n) ((time_t)(n) * 24 * 60 * 60)

static void nvd_init_client(http_client_t *client, config_t *config) {
    http_client_init(client);

    if (config->nvd_api_key != NULL) {
        char *key = NULL;
        EXIT_IF(asprintf(&key, "apiKey: %s", config->nvd_api_key) == -1, "asprintf");
        http_client_add_header(client, key);
        free(key);
    }
}

static void date_to_url(char *dst, size_t size, time_t date) {
    struct tm tm = {0};

    EXIT_IF(gmtime_r(&date, &tm) == NULL, "gmtime_r");
    EXIT_IF(strftime(dst, size, "%Y-%m-%dT%H:%M:%S.000Z", &tm) == 0, "strftime");
}

static char *parameters_to_url(int results_per_page, int start_index, time_t window_start, time_t window_end, unsigned cwe_id) {
    char start_url[32], end_url[32];
    date_to_url(start_url, sizeof(start_url), window_start);
    date_to_url(end_url, sizeof(end_url), window_end);

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

static int nvd_probe_window(http_client_t *client, time_t window_start, time_t window_end, unsigned cwe_id) {

    char *url = parameters_to_url(0, 0, window_start, window_end, cwe_id);

    yyjson_doc *doc = http_get_json(client, url);
    free(url);

    int result = nvd_parser_get_total_results(doc);
    yyjson_doc_free(doc);

    return result;
}

static void nvd_download_window(http_client_t *client, display_t *display, time_t window_start, time_t window_end, unsigned cwe_id) {

    display_download_start(display, cwe_id, window_start, window_end, 0, 0);
    int total_results = nvd_probe_window(client, window_start, window_end, cwe_id);
    display_download_complete(display);

    int total_pages = (total_results + RESULTS_PER_PAGE - 1) / RESULTS_PER_PAGE;
    for (int i = total_pages - 1; i >= 0; i--) {

        char *url = parameters_to_url(RESULTS_PER_PAGE, i * RESULTS_PER_PAGE, window_start, window_end, cwe_id);

        display_download_start(display, cwe_id, window_start, window_end, total_pages - i, total_pages);
        yyjson_doc *doc = http_get_json(client, url);
        display_download_complete(display);

        free(url);
#pragma omp task
        {
            nvd_parser_extract_commits(doc);
            yyjson_doc_free(doc);
        }
    }
}

void nvd_request(config_t *config, display_t *display) {
    http_client_t client = {0};
    nvd_init_client(&client, config);

    for (unsigned i = 0; i < config->cwe_ids_count; i++) {
        time_t window_end = config->cve_published_before;
        time_t window_start = window_end - DAYS(DAYS_PER_WINDOW) + 1;
        if (window_start < config->cve_published_after)
            window_start = config->cve_published_after;

        while (window_end > window_start) {
            nvd_download_window(&client, display, window_start, window_end, config->cwe_ids[i]);

            window_end -= DAYS(DAYS_PER_WINDOW);
            window_start -= DAYS(DAYS_PER_WINDOW);
            if (window_start < config->cve_published_after)
                window_start = config->cve_published_after;
        }
    }

    http_client_destroy(&client);
}
