#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "utils.h"

void exit_if(bool condition, const char *file, int line, const char *func, const char *fmt, ...) {
    if (!condition)
        return;

    fprintf(stderr, ERROR_C "[ERROR] %s:%d (%s): ", file, line, func);

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fprintf(stderr, RESET_C "\n");

    exit(EXIT_FAILURE);
}

void date_to_display_format(char *dst, size_t size, time_t date) {
    struct tm tm = {0};

    EXIT_IF(gmtime_r(&date, &tm) == NULL, "gmtime_r");
    EXIT_IF(strftime(dst, size, "%d/%m/%Y", &tm) == 0, "strftime");
}

void date_to_url_format(char *dst, size_t size, time_t date) {
    struct tm tm = {0};

    EXIT_IF(gmtime_r(&date, &tm) == NULL, "gmtime_r");
    EXIT_IF(strftime(dst, size, "%Y-%m-%dT%H:%M:%S.000Z", &tm) == 0, "strftime");
}
