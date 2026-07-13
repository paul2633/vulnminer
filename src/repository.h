#ifndef REPOSITORY_H
#define REPOSITORY_H

#include "config.h"

#include <stdlib.h>

typedef struct file {
    char *absolute_path;
    char *relative_path;
    char *name;
    struct file *next;
} file_t;

typedef struct {
    char *name;
    size_t file_count;
    file_t *files;
    file_t **ordered_files;
} repository_t;

void repository_init(repository_t *repo, config_t *config);

void repository_add_file(repository_t *repo, char *path, int offset_relative_path, int offset_name);

void repository_order_files(repository_t *repo);

void repository_destroy(repository_t *repo);

#endif
