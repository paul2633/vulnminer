#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "history.h"
#include "utils.h"

#define HISTORY_SECTIONS 3
#define SECTION_LEN 3

static history_section_t *section_new(const char *title) {
    history_section_t *section = calloc(1, sizeof(*section));
    EXIT_IF(section == NULL, "calloc");

    section->title = strdup(title);
    EXIT_IF(section->title == NULL, "strdup");

    section->lines = calloc(SECTION_LEN, sizeof(*section->lines));
    EXIT_IF(section->lines == NULL, "calloc");

    return section;
}

static void section_destroy(history_section_t *section) {
    free(section->title);
    for (int i = 0; i < SECTION_LEN; i++)
        free(section->lines[i]);
    free(section->lines);
    free(section);
}

history_t *history_new(void) {
    history_t *history = calloc(1, sizeof(*history));
    EXIT_IF(history == NULL, "calloc");

    history->previous_lines_count = -1;

    history->nvd_section = section_new("NVD CWE DOWNLOAD HISTORY");
    history->github_section = section_new("GITHUB COMMITS DOWNLOAD HISTORY");
    history->parsing_section = section_new("GITHUB COMMITS PARSING HISTORY");

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

static void display_section(history_t *history, const history_section_t *section) {
    printf("\033[2K" LOG_C "%s - %u pending | %u ongoing | %u succeeded | %u failed" RESET_C "\n",
           section->title,
           section->pending,
           section->ongoing,
           section->succeeded,
           section->failed);

    for (int i = SECTION_LEN - 1; i >= 0; i--) {
        if (section->lines[i] != NULL) {
            printf("\033[2K" LOG_C "%s" RESET_C "\n", section->lines[i]);
            history->previous_lines_count++;
        }
    }

    printf("\033[2K\n");
}

static void display_history(history_t *history) {
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

unsigned history_push(history_t *history, history_section_t *section, char *line, bool increment_ongoing) {
    pthread_mutex_lock(&history->lock);

    if (increment_ongoing) {
        section->pending--;
        section->ongoing++;
    }

    free(section->lines[SECTION_LEN - 1]);

    for (int i = SECTION_LEN - 1; i > 0; i--)
        section->lines[i] = section->lines[i - 1];

    section->lines[0] = line;
    unsigned line_number = ++section->pushed_lines_count;

    display_history(history);
    pthread_mutex_unlock(&history->lock);

    return line_number;
}

void history_append(history_t *history, history_section_t *section, unsigned line_number, const char *suffix, history_status_t succeeded) {
    pthread_mutex_lock(&history->lock);

    if (succeeded == HISTORY_STATUS_SUCCEEDED) {
        section->succeeded++;
        section->ongoing--;
    }

    else if (succeeded == HISTORY_STATUS_FAILED) {
        section->failed++;
        section->ongoing--;
    }

    unsigned line_index = section->pushed_lines_count - line_number;

    if (line_index < SECTION_LEN) {
        char *line = NULL;
        EXIT_IF(asprintf(&line, "%s%s", section->lines[line_index], suffix) == -1, "asprintf");

        free(section->lines[line_index]);
        section->lines[line_index] = line;
    }

    if (succeeded != HISTORY_STATUS_NONE || line_index < SECTION_LEN)
        display_history(history);

    pthread_mutex_unlock(&history->lock);
}

void history_set_pending(history_t *history, history_section_t *section, unsigned pending) {
    pthread_mutex_lock(&history->lock);
    section->pending = pending;
    display_history(history);
    pthread_mutex_unlock(&history->lock);
}

void history_increment_pending(history_t *history, history_section_t *section) {
    pthread_mutex_lock(&history->lock);
    section->pending++;
    display_history(history);
    pthread_mutex_unlock(&history->lock);
}
