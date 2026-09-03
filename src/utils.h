#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>

#define RESET_C "\033[0m"
#define ERROR_C "\033[0m\033[31m"
#define LOG_C "\033[0m\033[36m"

#define EXIT_IF(cond, ...) exit_if((cond), __FILE__, __LINE__, __func__, __VA_ARGS__)

void exit_if(bool condition, const char *file, int line, const char *func, const char *fmt, ...);

#endif
