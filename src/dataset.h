#ifndef DATASET_H
#define DATASET_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    char *name;
    char *content;

    char *function_type;
    char **parameters_types;
    unsigned parameters_count;
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
    char *path;
    unsigned distance;
} dataset_context_distance_t;

typedef struct {
    char *path;
    char *content;
    size_t size;

    dataset_context_distance_t **distances;
    unsigned distances_count;
} dataset_context_file_t;

typedef struct {
    unsigned cwe_id;

    char *cve_id;
    char *cve_published;
    char *cve_description;

    char *repo_name;
    char *commit_hash;
    char *parent_commit_hash;
    char *commit_message;

    dataset_file_t **files;
    unsigned files_count;

    dataset_context_file_t **context_files;
    unsigned context_files_count;
} dataset_entry_t;

void add_before_function(dataset_file_t *file, const char *function_name, uint32_t function_start, uint32_t function_end);

void add_after_function(dataset_file_t *file, const char *function_name, uint32_t function_start, uint32_t function_end);

void add_new_file(dataset_entry_t *entry, const char *path, const char *previous_path, const char *status);

void add_new_context_distance(dataset_context_file_t *context_file, const char *path, unsigned distance);

dataset_context_file_t *add_and_get_new_context_file(dataset_entry_t *entry, const char *path);

dataset_entry_t *dataset_entry_new(unsigned cwe_id, const char *cve_id, const char *repo_name, const char *commit_hash, const char *cve_description,
                                   const char *cve_published);

void dataset_entry_destroy(dataset_entry_t *entry);

#endif
