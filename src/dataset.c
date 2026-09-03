#include <stdlib.h>
#include <string.h>

#include "dataset.h"
#include "utils.h"

dataset_file_t *dataset_file_new(const char *path) {
    dataset_file_t *file = calloc(1, sizeof(*file));
    EXIT_IF(file == NULL, "calloc");

    file->path = strdup(path);
    EXIT_IF(file->path == NULL, "strdup");

    return file;
}

static void dataset_file_destroy(dataset_file_t *file) {
    free(file->path);
    free(file->before);
    free(file->after);
    free(file);
}

dataset_entry_t *dataset_entry_new(unsigned cwe_id, const char *cve_id, const char *repo_name, const char *commit_hash) {
    dataset_entry_t *entry = calloc(1, sizeof(*entry));
    EXIT_IF(entry == NULL, "calloc");

    entry->cwe_id = cwe_id;

    entry->cve_id = strdup(cve_id);
    EXIT_IF(entry->cve_id == NULL, "strdup");

    entry->repo_name = strdup(repo_name);
    EXIT_IF(entry->repo_name == NULL, "strdup");

    entry->commit_hash = strdup(commit_hash);
    EXIT_IF(entry->commit_hash == NULL, "strdup");

    return entry;
}

void dataset_entry_destroy(dataset_entry_t *entry) {
    free(entry->cve_id);
    free(entry->cve_description);

    free(entry->repo_name);
    free(entry->commit_hash);
    free(entry->parent_commit_hash);
    free(entry->commit_message);

    for (unsigned i = 0; i < entry->files_count; i++)
        dataset_file_destroy(entry->files[i]);

    free(entry->files);
    free(entry);
}
