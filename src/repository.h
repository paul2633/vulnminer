#ifndef REPOSITORY_H
#define REPOSITORY_H

#include <stdlib.h>

typedef enum { FILE_C, FILE_HEADER } file_type_t;

typedef struct file {
    char *name;
    char *absolute_path;
    char *relative_path;
    file_type_t type;
    size_t line_count;
    struct file *next;
} file_t;

typedef enum { MODE_NONE, MODE_LOCAL, MODE_DOWNLOAD, MODE_REMOTE } repo_mode_t;

typedef struct {
    repo_mode_t mode;
    char *name;
    char *absolute_path;
    char *url;
    size_t c_file_count;
    size_t header_file_count;
    file_t *files;
} repository_t;

void repository_init(repository_t *repo);

void repository_add_file(repository_t *repo, const char *path, file_type_t type);

void repository_destroy_file(file_t *file);

void repository_destroy(repository_t *repo);

#endif
