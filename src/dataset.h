#ifndef DATASET_H
#define DATASET_H

#include <stddef.h>

typedef enum {
    ONGOING = 0,
    FILE_NOT_MODIFIED,
    EXTENSION_NOT_SUPPORTED,
    FILE_CONTENT_UNAVAILABLE,
    NO_MODIFIED_FUNCTION,
} file_state_t;

typedef struct {
    char *path;
    file_state_t state;

    char *before;
    size_t before_size;

    char *after;
    size_t after_size;
} dataset_file_t;

typedef struct {
    unsigned cwe_id;
    char *cve_id;
    char *repo_name;
    char *commit_hash;

    char *cve_description;
    char *parent_commit_hash;
    char *commit_message;

    dataset_file_t **files;
    unsigned files_count;
} dataset_entry_t;

dataset_file_t *dataset_file_new(const char *path);

dataset_entry_t *dataset_entry_new(unsigned cwe_id, const char *cve_id, const char *repo_name, const char *commit_hash);

void dataset_entry_destroy(dataset_entry_t *entry);

#endif
