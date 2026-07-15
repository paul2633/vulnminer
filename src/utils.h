#ifndef UTILS_H
#define UTILS_H

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "config.h"

#define RESET_C "\033[0m"
#define ERROR_C "\033[0m\033[31m"
#define LOG_C "\033[0m\033[36m"

void exit_if(int condition, const char *fn_name, const char *msg);

void remove_directory(char *path);

void download_repository(char *source, char *commit, char *target);

void json_write(FILE *f, unsigned indent, const char *fmt, ...);

void output_perf_numbers(const config_t *config, double time_s, double memory_gb);

#endif
