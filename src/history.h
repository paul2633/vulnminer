#ifndef HISTORY_H
#define HISTORY_H

#include <pthread.h>
#include <stdbool.h>

typedef struct {
    char *title;
    unsigned pending;
    unsigned ongoing;
    unsigned succeeded;
    unsigned failed;
    char **lines;
    unsigned pushed_lines_count;
} history_section_t;

typedef struct {
    history_section_t *nvd_section;
    history_section_t *github_section;
    history_section_t *parsing_section;
    int previous_lines_count;
    pthread_mutex_t lock;
} history_t;

typedef enum { HISTORY_STATUS_NONE = -1, HISTORY_STATUS_FAILED = 0, HISTORY_STATUS_SUCCEEDED = 1 } history_status_t;

history_t *history_new(void);

void history_destroy(history_t *history);

unsigned history_push(history_t *history, history_section_t *section, char *line, bool increment_ongoing);

void history_append(history_t *history, history_section_t *section, unsigned line_number, const char *suffix, history_status_t succeeded);

void history_set_pending(history_t *history, history_section_t *section, unsigned pending);

void history_increment_pending(history_t *history, history_section_t *section);

#endif
