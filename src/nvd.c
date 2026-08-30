#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <yyjson.h>

#include "config.h"
#include "history.h"
#include "http.h"
#include "jobs.h"
#include "nvd.h"
#include "nvd_parser.h"
#include "utils.h"

#define NVD_API "https://services.nvd.nist.gov/rest/json/cves/2.0"
#define RESULTS_PER_PAGE 100
#define DAYS_PER_WINDOW 120

#define DAYS(n) ((time_t)(n) * 24 * 60 * 60)

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

static int probe_window(http_client_t *client, time_t window_start, time_t window_end, unsigned cwe_id) {
    char *url = parameters_to_url(0, 0, window_start, window_end, cwe_id);

    yyjson_doc *doc = http_get_json(client, url);
    free(url);

    EXIT_IF(doc == NULL, "HTTP error 404");

    int result = nvd_parser_get_total_results(doc);
    yyjson_doc_free(doc);

    return result;
}

static char *download_line(unsigned cwe_id, const char *start, const char *end, int page, int total_pages) {
    char *line = NULL;

    if (total_pages == 0)
        EXIT_IF(asprintf(&line, "CWE-%-3u   %s-%s   %-25s", cwe_id, start, end, "probing...") == -1, "asprintf");

    else {
        char *pages = NULL;
        EXIT_IF(asprintf(&pages, "fetching page %d/%d...", page, total_pages) == -1, "asprintf");
        EXIT_IF(asprintf(&line, "CWE-%-3u   %s-%s   %-25s", cwe_id, start, end, pages) == -1, "asprintf");
        free(pages);
    }

    return line;
}

static void download_window(http_client_t *client, jobs_queue_t *queue, history_t *history, time_t window_start, time_t window_end, unsigned cwe_id) {
    char start_display[11], end_display[11];
    date_to_display(start_display, sizeof(start_display), window_start);
    date_to_display(end_display, sizeof(end_display), window_end);

    unsigned line_number = history_push(history, history->nvd_section, download_line(cwe_id, start_display, end_display, 0, 0));
    int total_results = probe_window(client, window_start, window_end, cwe_id);
    history_append(history, history->nvd_section, line_number, "complete");

    int total_pages = (total_results + RESULTS_PER_PAGE - 1) / RESULTS_PER_PAGE;

    for (int i = total_pages - 1; i >= 0; i--) {

        char *url = parameters_to_url(RESULTS_PER_PAGE, i * RESULTS_PER_PAGE, window_start, window_end, cwe_id);

        line_number = history_push(history, history->nvd_section, download_line(cwe_id, start_display, end_display, total_pages - i, total_pages));
        yyjson_doc *doc = http_get_json(client, url);
        free(url);

        EXIT_IF(doc == NULL, "HTTP error 404");

        history_append(history, history->nvd_section, line_number, "complete");

        nvd_parser_extract_commits(doc, queue);
        yyjson_doc_free(doc);
    }
}

void nvd_request(const config_t *config, jobs_queue_t *queue, history_t *history) {
    http_client_t *nvd_client = nvd_client_new(config->nvd_api_key);

    for (unsigned i = 0; i < config->cwe_ids_count; i++) {

        time_t window_end = config->cve_published_before;
        time_t window_start = window_end - DAYS(DAYS_PER_WINDOW) + 1;
        if (window_start < config->cve_published_after)
            window_start = config->cve_published_after;

        while (window_end > window_start) {

            download_window(nvd_client, queue, history, window_start, window_end, config->cwe_ids[i]);

            window_end -= DAYS(DAYS_PER_WINDOW);
            window_start -= DAYS(DAYS_PER_WINDOW);
            if (window_start < config->cve_published_after)
                window_start = config->cve_published_after;
        }
    }

    http_client_destroy(nvd_client);
}
