#ifndef SCANNER_H
#define SCANNER_H

#include "repository.h"

static void scan_directory(repository_t *repo, const char *path);

void scanner_scan(repository_t *repo);

#endif