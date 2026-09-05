#ifndef DATASET_H
#define DATASET_H

#include <stddef.h>

typedef struct {
    char *path;
    char *previous_path;
    char *status;

    char *before;
    size_t before_size;

    char *after;
    size_t after_size;
} dataset_file_t;

typedef struct {
    unsigned cwe_id;

    char *cve_id;
    char *cve_description;

    char *repo_name;
    char *commit_message;
    char *commit_hash;
    char *parent_commit_hash;

    dataset_file_t **files;
    unsigned files_count;
} dataset_entry_t;

dataset_file_t *dataset_file_new(const char *path, const char *previous_path, const char *status);

dataset_entry_t *dataset_entry_new(unsigned cwe_id, const char *cve_id, const char *repo_name, const char *commit_hash, const char *cve_description);

void dataset_entry_destroy(dataset_entry_t *entry);

#endif
