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

void display_download_start(display_t *display, unsigned cwe_id, time_t window_start, time_t window_end, int page, int total_pages);

void display_download_complete(display_t *display);

void display_destroy(display_t *display);

#endif
