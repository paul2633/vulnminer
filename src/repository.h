#ifndef REPOSITORY_H
#define REPOSITORY_H

#include <stdlib.h>

typedef enum {
    FILE_C,
    FILE_HEADER
} file_type_t;

typedef struct file {
    char *path;
    file_type_t type;
    size_t line_count;
    struct file *next;
} file_t;

typedef struct {
    char *path;
    size_t c_file_count;
    size_t header_file_count;
    file_t *files;

} repository_t;

void repository_init(repository_t *repo, char *path);

void repository_destroy(repository_t *repo);

#endif