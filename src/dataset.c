#include <stdlib.h>
#include <string.h>

#include "dataset.h"
#include "utils.h"

static void set_string(char **dst, const char *new) {
    *dst = strdup(new);
    EXIT_IF(*dst == NULL, "strdup");
}

static dataset_function_t *dataset_function_new(const char *function_name, const char *file_content, uint32_t function_start, uint32_t function_end) {
    dataset_function_t *function = calloc(1, sizeof(*function));
    EXIT_IF(function == NULL, "calloc");

    set_string(&function->name, function_name);

    size_t size = function_end - function_start;
    function->content = malloc(size + 1);
    EXIT_IF(function->content == NULL, "malloc");

    memcpy(function->content, file_content + function_start, size);
    function->content[size] = '\0';
    return function;
}

void add_before_function(dataset_file_t *file, const char *function_name, uint32_t function_start, uint32_t function_end) {
    file->before_functions = realloc(file->before_functions, (file->before_functions_count + 1) * sizeof(*file->before_functions));
    EXIT_IF(file->before_functions == NULL, "realloc");
    file->before_functions[file->before_functions_count++] = dataset_function_new(function_name, file->before, function_start, function_end);
}

void add_after_function(dataset_file_t *file, const char *function_name, uint32_t function_start, uint32_t function_end) {
    file->after_functions = realloc(file->after_functions, (file->after_functions_count + 1) * sizeof(*file->after_functions));
    EXIT_IF(file->after_functions == NULL, "realloc");
    file->after_functions[file->after_functions_count++] = dataset_function_new(function_name, file->after, function_start, function_end);
}

static void dataset_function_destroy(dataset_function_t *function) {
    free(function->name);
    free(function->content);
    free(function->function_type);

    for (unsigned i = 0; i < function->parameters_count; i++)
        free(function->parameters_types[i]);
    free(function->parameters_types);

    free(function);
}

void add_new_file(dataset_entry_t *entry, const char *path, const char *previous_path, const char *status) {
    entry->files = realloc(entry->files, (entry->files_count + 1) * sizeof(*entry->files));
    EXIT_IF(entry->files == NULL, "realloc");

    dataset_file_t *file = calloc(1, sizeof(*file));
    EXIT_IF(file == NULL, "calloc");

    if (path != NULL)
        set_string(&file->path, path);

    if (previous_path != NULL)
        set_string(&file->previous_path, previous_path);

    set_string(&file->status, status);
    entry->files[entry->files_count++] = file;
}

static void dataset_file_destroy(dataset_file_t *file) {
    free(file->path);
    free(file->previous_path);
    free(file->status);

    free(file->before);
    free(file->after);

    for (unsigned i = 0; i < file->before_functions_count; i++)
        dataset_function_destroy(file->before_functions[i]);
    free(file->before_functions);

    for (unsigned i = 0; i < file->after_functions_count; i++)
        dataset_function_destroy(file->after_functions[i]);
    free(file->after_functions);

    free(file);
}

void add_new_context_file(dataset_entry_t *entry, const char *path) {
    entry->context_files = realloc(entry->context_files, (entry->context_files_count + 1) * sizeof(*entry->context_files));
    EXIT_IF(entry->context_files == NULL, "realloc");

    dataset_context_file_t *context_file = calloc(1, sizeof(*context_file));
    EXIT_IF(context_file == NULL, "calloc");

    set_string(&context_file->path, path);
    entry->context_files[entry->context_files_count++] = context_file;
}

static void dataset_context_file_destroy(dataset_context_file_t *context_file) {
    free(context_file->path);
    free(context_file->content);
    free(context_file);
}

dataset_entry_t *dataset_entry_new(unsigned cwe_id, const char *cve_id, const char *repo_name, const char *commit_hash, const char *cve_description,
                                   const char *cve_published) {
    dataset_entry_t *entry = calloc(1, sizeof(*entry));
    EXIT_IF(entry == NULL, "calloc");

    entry->cwe_id = cwe_id;

    set_string(&entry->cve_id, cve_id);
    set_string(&entry->cve_published, cve_published);
    if (cve_description != NULL)
        set_string(&entry->cve_description, cve_description);

    set_string(&entry->repo_name, repo_name);
    set_string(&entry->commit_hash, commit_hash);

    return entry;
}

void dataset_entry_destroy(dataset_entry_t *entry) {
    free(entry->cve_id);
    free(entry->cve_published);
    free(entry->cve_description);

    free(entry->repo_name);
    free(entry->commit_hash);
    free(entry->parent_commit_hash);
    free(entry->commit_message);

    for (unsigned i = 0; i < entry->files_count; i++)
        dataset_file_destroy(entry->files[i]);
    free(entry->files);

    for (unsigned i = 0; i < entry->context_files_count; i++)
        dataset_context_file_destroy(entry->context_files[i]);
    free(entry->context_files);

    free(entry);
}
