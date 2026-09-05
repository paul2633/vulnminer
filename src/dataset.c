#include <stdlib.h>
#include <string.h>

#include "dataset.h"
#include "utils.h"

dataset_file_t *dataset_file_new(const char *path, const char *previous_path, const char *status) {
    dataset_file_t *file = calloc(1, sizeof(*file));
    EXIT_IF(file == NULL, "calloc");

    if (path != NULL) {
        file->path = strdup(path);
        EXIT_IF(file->path == NULL, "strdup");
    }

    if (previous_path != NULL) {
        file->previous_path = strdup(previous_path);
        EXIT_IF(file->previous_path == NULL, "strdup");
    }

    if (status != NULL) {
        file->status = strdup(status);
        EXIT_IF(file->status == NULL, "strdup");
    }

    return file;
}

static void dataset_file_destroy(dataset_file_t *file) {
    if (file == NULL)
        return;

    free(file->path);
    free(file->previous_path);
    free(file->status);
    free(file->before);
    free(file->after);
    free(file);
}

dataset_entry_t *dataset_entry_new(unsigned cwe_id, const char *cve_id, const char *repo_name, const char *commit_hash, const char *cve_description) {
    dataset_entry_t *entry = calloc(1, sizeof(*entry));
    EXIT_IF(entry == NULL, "calloc");

    entry->cwe_id = cwe_id;

    if (cve_id != NULL) {
        entry->cve_id = strdup(cve_id);
        EXIT_IF(entry->cve_id == NULL, "strdup");
    }

    if (repo_name != NULL) {
        entry->repo_name = strdup(repo_name);
        EXIT_IF(entry->repo_name == NULL, "strdup");
    }

    if (commit_hash != NULL) {
        entry->commit_hash = strdup(commit_hash);
        EXIT_IF(entry->commit_hash == NULL, "strdup");
    }

    if (cve_description != NULL) {
        entry->cve_description = strdup(cve_description);
        EXIT_IF(entry->cve_description == NULL, "strdup");
    }

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
