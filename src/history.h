#ifndef HISTORY_H
#define HISTORY_H

#include <pthread.h>
#include <stdbool.h>

typedef struct {
    char *prefix;
    char *suffix;
} history_line_t;

typedef struct {
    char *title;
    unsigned pending;
    unsigned ongoing;
    unsigned done;
    history_line_t **lines;
    unsigned pushed_lines_count;
} history_section_t;

typedef struct {
    history_section_t *nvd_section;
    history_section_t *github_section;
    history_section_t *parsing_section;
    int previous_lines_count;
    pthread_mutex_t lock;
} history_t;

history_t *history_new(void);

void history_destroy(history_t *history);

void history_set_pending(history_t *history, history_section_t *section, unsigned value);

void history_increment_pending(history_t *history, history_section_t *section);

unsigned history_add_line(history_t *history, history_section_t *section, const char *prefix, const char *suffix);

void history_update_line(history_t *history, history_section_t *section, unsigned line_number, const char *suffix, bool done);

#endif
