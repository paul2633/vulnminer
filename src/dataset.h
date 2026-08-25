#ifndef DATASET_H
#define DATASET_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    char *name;
    uint32_t start_byte;
    uint32_t end_byte;
} dataset_function_t;

typedef struct {
    char *path;
    char *previous_path;
    char *status;

    char *before;
    size_t before_size;

    char *after;
    size_t after_size;

    dataset_function_t **before_functions;
    unsigned before_functions_count;

    dataset_function_t **after_functions;
    unsigned after_functions_count;
} dataset_file_t;

typedef struct {
    unsigned cwe_id;

    char *cve_id;
    char *cve_published;
    char *cve_description;

    char *repo_name;
    char *commit_message;
    char *commit_hash;
    char *parent_commit_hash;

    dataset_file_t **files;
    unsigned files_count;
} dataset_entry_t;

void dataset_file_add_before_function(dataset_file_t *file, const char *name, uint32_t start_byte, uint32_t end_byte);

void dataset_file_add_after_function(dataset_file_t *file, const char *name, uint32_t start_byte, uint32_t end_byte);

dataset_file_t *dataset_file_new(const char *path, const char *previous_path, const char *status);

dataset_entry_t *dataset_entry_new(unsigned cwe_id, const char *cve_id, const char *repo_name, const char *commit_hash, const char *cve_description,
                                   const char *cve_published);

void dataset_entry_destroy(dataset_entry_t *entry);

#endif
