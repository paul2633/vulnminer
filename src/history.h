#ifndef HISTORY_H
#define HISTORY_H

#include <pthread.h>

typedef struct {
    char **nvd_section;
    char **github_section;
    char **parsing_section;
    int previous_lines_count;

    pthread_mutex_t lock;
} history_t;

void display_history(history_t *history);

history_t *history_new(void);

void history_destroy(history_t *history);

void history_push(history_t *history, char **section, char *line);

void history_append(history_t *history, char **section, const char *suffix);

#endif
