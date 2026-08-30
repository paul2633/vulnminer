#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "history.h"
#include "utils.h"

#define HISTORY_SECTIONS 3

static history_section_t *section_new(const char *title) {
    history_section_t *section = calloc(1, sizeof(*section));
    EXIT_IF(section == NULL, "calloc");

    section->title = strdup(title);
    EXIT_IF(section->title == NULL, "strdup");

    return section;
}

static void section_destroy(history_section_t *section) {
    free(section->title);
    for (int i = 0; i < SECTION_LEN; i++)
        free(section->lines[i]);
    free(section);
}

history_t *history_new(void) {
    history_t *history = calloc(1, sizeof(*history));
    EXIT_IF(history == NULL, "calloc");

    history->previous_lines_count = -1;

    history->nvd_section = section_new("NVD DOWNLOAD HISTORY");
    history->github_section = section_new("GITHUB DOWNLOAD HISTORY");
    history->parsing_section = section_new("PARSING HISTORY");

    EXIT_IF(pthread_mutex_init(&history->lock, NULL) != 0, "pthread_mutex_init");

    return history;
}

void history_destroy(history_t *history) {
    pthread_mutex_destroy(&history->lock);

    section_destroy(history->nvd_section);
    section_destroy(history->github_section);
    section_destroy(history->parsing_section);

    free(history);
}

static void display_section(history_t *history, history_section_t *section) {
    printf("\033[2K" LOG_C "%s" RESET_C "\n", section->title);

    for (int i = SECTION_LEN - 1; i >= 0; i--) {
        if (section->lines[i] != NULL) {
            printf("\033[2K" LOG_C "%s" RESET_C "\n", section->lines[i]);
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

    display_section(history, history->nvd_section);
    display_section(history, history->github_section);
    display_section(history, history->parsing_section);
    fflush(stdout);
}

unsigned history_push(history_t *history, history_section_t *section, char *line) {
    pthread_mutex_lock(&history->lock);
    free(section->lines[SECTION_LEN - 1]);

    for (int i = SECTION_LEN - 1; i > 0; i--)
        section->lines[i] = section->lines[i - 1];

    section->lines[0] = line;
    unsigned line_number = ++section->pushed_lines_count;

    display_history(history);
    pthread_mutex_unlock(&history->lock);

    return line_number;
}

void history_append(history_t *history, history_section_t *section, unsigned line_number, const char *suffix) {
    pthread_mutex_lock(&history->lock);

    unsigned line_index = section->pushed_lines_count - line_number;

    if (line_index < SECTION_LEN) {
        char *line = NULL;
        EXIT_IF(asprintf(&line, "%s%s", section->lines[line_index], suffix) == -1, "asprintf");

        free(section->lines[line_index]);
        section->lines[line_index] = line;

        display_history(history);
    }

    pthread_mutex_unlock(&history->lock);
}
