#ifndef HISTORY_H
#define HISTORY_H

#include <pthread.h>

typedef struct {
    char *title;
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

void display_history(history_t *history);

history_t *history_new(void);

void history_destroy(history_t *history);

unsigned history_push(history_t *history, history_section_t *section, char *line);

void history_append(history_t *history, history_section_t *section, unsigned line_number, const char *suffix);

#endif
