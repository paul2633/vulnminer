#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

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
