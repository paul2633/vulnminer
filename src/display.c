#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "display.h"
#include "utils.h"

#define HISTORY_LEN 5 // > 0

static void date_to_display(char *dst, size_t size, time_t date) {
    struct tm tm = {0};

    EXIT_IF(gmtime_r(&date, &tm) == NULL, "gmtime_r");
    EXIT_IF(strftime(dst, size, "%d/%m/%Y", &tm) == 0, "strftime");
}

void display_config(const config_t *config) {
    char before_display[11], after_display[11];
    date_to_display(before_display, sizeof(before_display), config->cve_published_before);
    date_to_display(after_display, sizeof(after_display), config->cve_published_after);

    printf(LOG_C "[INIT] cwe-ids: ");
    for (unsigned i = 0; i < config->cwe_ids_count; i++) {
        printf("%u", config->cwe_ids[i]);
        if (i < config->cwe_ids_count - 1)
            printf(", ");
    }
    printf(RESET_C "\n");
    printf(LOG_C "[INIT] cve-published-before: %s" RESET_C "\n", before_display);
    printf(LOG_C "[INIT] cve-published-after: %s" RESET_C "\n", after_display);
    printf(LOG_C "[INIT] nvd_api_key: %s" RESET_C "\n", config->nvd_api_key);
    printf(LOG_C "[INIT] github_api_key: %s" RESET_C "\n", config->github_api_key);
}

static void display_print(display_t *display) {

    if (display->previous_display_lines_count > 0) {
        printf("\033[%uA", display->previous_display_lines_count);
        display->previous_display_lines_count = 0;
    }

    printf("\033[4A");
    printf("\033[2K" LOG_C "NVD DOWNLOAD HISTORY" RESET_C "\n");

    for (int i = HISTORY_LEN - 1; i >= 0; i--) {
        if (display->download_history[i] != NULL) {
            printf("\033[2K" LOG_C "%s" RESET_C "\n", display->download_history[i]);
            display->previous_display_lines_count++;
        }
    }

    printf("\033[2K\n");
    printf("\033[2K" LOG_C "GITHUB DOWNLOAD HISTORY" RESET_C "\n");
    printf("\033[2K" LOG_C "%s" RESET_C "\n", display->parsing);

    fflush(stdout);
}

void display_init(display_t *display) {
    memset(display, 0, sizeof(*display));

    display->download_history = calloc(HISTORY_LEN, sizeof(*display->download_history));
    EXIT_IF(display->download_history == NULL, "calloc");

    display->parsing = strdup("   TODO");
    EXIT_IF(display->parsing == NULL, "strdup");

    printf("\n\n\n\n\n");
    display_print(display);
}

void display_download_start(display_t *display, unsigned cwe_id, time_t window_start, time_t window_end, int page, int total_pages) {

    free(display->download_history[HISTORY_LEN - 1]);
    for (int i = HISTORY_LEN - 1; i > 0; i--)
        display->download_history[i] = display->download_history[i - 1];

    char start_display[11], end_display[11];
    date_to_display(start_display, sizeof(start_display), window_start);
    date_to_display(end_display, sizeof(end_display), window_end);

    char *line = NULL;

    if (total_pages == 0)
        EXIT_IF(asprintf(&line, "   CWE-%-3u   %s-%s   probing...", cwe_id, start_display, end_display) == -1, "asprintf");
    else
        EXIT_IF(asprintf(&line, "   CWE-%-3u   %s-%s   fetching page %d/%d...", cwe_id, start_display, end_display, page, total_pages) == -1, "asprintf");

    display->download_history[0] = line;
    display_print(display);
}

void display_download_complete(display_t *display) {
    EXIT_IF(display->download_history[0] == NULL, "display_download_complete");

    char *line;
    EXIT_IF(asprintf(&line, "%-60s complete", display->download_history[0]) == -1, "asprintf");

    free(display->download_history[0]);
    display->download_history[0] = line;
    display_print(display);
}

void display_destroy(display_t *display) {
    for (unsigned i = 0; i < HISTORY_LEN; i++)
        free(display->download_history[i]);
    free(display->download_history);
    free(display->parsing);
}
