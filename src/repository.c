#include <string.h>

#include "repository.h"
#include "utils.h"

void repository_add_file(repository_t *repo, const char *path, file_type_t type) {
    file_t *file = malloc(sizeof(file_t));
    exit_if(file == NULL, "malloc");
    memset(file, 0, sizeof(*file));

    file->path = strdup(path);
    exit_if(file->path == NULL, "strdup");

    file->type = type;

    // critical section
    file->next = repo->files;
    repo->files = file;
    // critical section end
}

void repository_init(repository_t *repo, const char *path) {
    memset(repo, 0, sizeof(*repo));
    repo->path = strdup(path);
    exit_if(repo->path == NULL, "strdup");
}

void repository_destroy_file(file_t *file) {
    free(file->path);
    free(file);
}

void repository_destroy(repository_t *repo) {
    while (repo->files != NULL) {
        file_t *tmp = repo->files;
        repo->files = repo->files->next;
        repository_destroy_file(tmp);
    }

    free(repo->path);
}
