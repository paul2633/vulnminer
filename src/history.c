#include <stdio.h>
#include <stdlib.h>

#include "history.h"
#include "utils.h"

#define HISTORY_SECTIONS 3
#define HISTORY_LEN 3

history_t *history_new(void) {
    history_t *history = calloc(1, sizeof(*history));
    EXIT_IF(history == NULL, "calloc");

    history->previous_lines_count = -1;

    history->nvd_section = calloc(HISTORY_LEN, sizeof(*history->nvd_section));
    EXIT_IF(history->nvd_section == NULL, "calloc");

    history->github_section = calloc(HISTORY_LEN, sizeof(*history->github_section));
    EXIT_IF(history->github_section == NULL, "calloc");

    history->parsing_section = calloc(HISTORY_LEN, sizeof(*history->parsing_section));
    EXIT_IF(history->parsing_section == NULL, "calloc");

    EXIT_IF(pthread_mutex_init(&history->lock, NULL) != 0, "pthread_mutex_init");

    return history;
}

void history_destroy(history_t *history) {
    pthread_mutex_destroy(&history->lock);

    for (int i = 0; i < HISTORY_LEN; i++) {
        free(history->nvd_section[i]);
        free(history->github_section[i]);
        free(history->parsing_section[i]);
    }

    free(history->nvd_section);
    free(history->github_section);
    free(history->parsing_section);

    free(history);
}

static void display_section(history_t *history, char **section, const char *title) {
    printf("\033[2K" LOG_C "%s" RESET_C "\n", title);

    for (int i = HISTORY_LEN - 1; i >= 0; i--) {
        if (section[i] != NULL) {
            printf("\033[2K" LOG_C "%s" RESET_C "\n", section[i]);
            history->previous_lines_count++;
        }
    }

    printf("\033[2K\n");
}

void display_history(history_t *history) {
    if (history->previous_lines_count < 0)
        for (int i = 0; i < HISTORY_SECTIONS; i++)
            printf("\n\n");

    else if (history->previous_lines_count > 0)
        printf("\033[%dA", history->previous_lines_count);

    printf("\033[%dA", HISTORY_SECTIONS * 2);

    history->previous_lines_count = 0;

    display_section(history, history->nvd_section, "NVD DOWNLOAD HISTORY");
    display_section(history, history->github_section, "GITHUB DOWNLOAD HISTORY");
    display_section(history, history->parsing_section, "PARSING HISTORY");
    fflush(stdout);
}

void history_push(history_t *history, char **section, char *line) {
    pthread_mutex_lock(&history->lock);
    free(section[HISTORY_LEN - 1]);

    for (int i = HISTORY_LEN - 1; i > 0; i--)
        section[i] = section[i - 1];

    section[0] = line;

    display_history(history);
    pthread_mutex_unlock(&history->lock);
}

void history_append(history_t *history, char **section, const char *suffix) {
    pthread_mutex_lock(&history->lock);
    char *line = NULL;

    EXIT_IF(asprintf(&line, "%s   %s", section[0], suffix) == -1, "asprintf");

    free(section[0]);
    section[0] = line;

    display_history(history);
    pthread_mutex_unlock(&history->lock);
}
