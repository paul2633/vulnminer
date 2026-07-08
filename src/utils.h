#ifndef UTILS_H
#define UTILS_H

#include "repository.h"

#define RESET_C "\033[0m"
#define ERROR_C "\033[0m\033[31m"

void exit_if(int condition, const char *msg);

void remove_path(char *path);

void download_repository(const repository_t *repo);

#endif
