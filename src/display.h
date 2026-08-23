#ifndef DISPLAY_H
#define DISPLAY_H

#include <time.h>

#include "config.h"

typedef struct {
    char **download_history;
    unsigned previous_display_lines_count;
    char *parsing;
} display_t;

void display_config(const config_t *config);

void display_init(display_t *display);

void display_update(display_t *display, unsigned cwe_id, unsigned page, unsigned total_pages, time_t window_start, time_t window_end);

void display_destroy(display_t *display);

#endif
