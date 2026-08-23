#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "display.h"
#include "utils.h"

#define HISTORY_LEN 3 // > 0

void display_config(const config_t *config) {
    printf(LOG_C "[INIT] cve-published-before: %s\n", config->cve_published_before_str);
    printf("[INIT] cve-published-after: %s\n", config->cve_published_after_str);
    printf("[INIT] max-commits-per-cwe: %u\n", config->max_commits_per_cwe);
    printf("[INIT] nvd_api_key: %s\n", config->nvd_api_key);
    printf("[INIT] cwe-ids: [ ");
    for (unsigned i = 0; i < config->cwe_ids_count; i++)
        printf("%u ", config->cwe_ids[i]);
    printf("]\n[INIT] threads-count: %u" RESET_C "\n", config->threads_count);
}

static void display_print(display_t *display) {

    if (display->previous_display_lines_count > 0) {
        printf("\033[%uA", display->previous_display_lines_count);
        display->previous_display_lines_count = 0;
    }

    printf("\033[4A");
    printf("\033[2K" LOG_C "DOWNLOAD HISTORY" RESET_C "\n");

    for (int i = HISTORY_LEN - 1; i >= 0; i--) {
        if (display->download_history[i] != NULL) {
            printf("\033[2K" LOG_C "%s" RESET_C "\n", display->download_history[i]);
            display->previous_display_lines_count++;
        }
    }

    printf("\033[2K"
           "\n");
    printf("\033[2K" LOG_C "PARSING" RESET_C "\n");
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

void display_update(display_t *display, unsigned cwe_id, unsigned page, unsigned total_pages, time_t window_start, time_t window_end) {
    if (display->download_history[0] != NULL) {
        char *line;
        EXIT_IF(asprintf(&line, "%s   done", display->download_history[0]) == -1, "asprintf");
        free(display->download_history[0]);
        display->download_history[0] = line;
    }

    free(display->download_history[HISTORY_LEN - 1]);
    for (int i = HISTORY_LEN - 1; i > 0; i--)
        display->download_history[i] = display->download_history[i - 1];

    char window_start_display[11], window_end_display[11];
    date_to_display_format(window_start_display, sizeof(window_start_display), window_start);
    date_to_display_format(window_end_display, sizeof(window_end_display), window_end);

    char *line = NULL;

    if (total_pages == 0)
        EXIT_IF(asprintf(&line, "   CWE-%-3u   %s-%s   probing...", cwe_id, window_start_display, window_end_display) == -1, "asprintf");
    else
        EXIT_IF(asprintf(&line, "   CWE-%-3u   %s-%s   fetching page %d/%d...", cwe_id, window_start_display, window_end_display, page, total_pages) == -1,
                "asprintf");

    display->download_history[0] = line;
    display_print(display);
}

void display_destroy(display_t *display) {
    for (unsigned i = 0; i < HISTORY_LEN; i++)
        free(display->download_history[i]);
    free(display->download_history);
    free(display->parsing);
}
