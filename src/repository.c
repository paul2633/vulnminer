#include <string.h>

#include "repository.h"
#include "utils.h"

void repository_init(repository_t *repo) { memset(repo, 0, sizeof(*repo)); }

void repository_add_file(repository_t *repo, const char *path, file_type_t type) {
    file_t *file = malloc(sizeof(file_t));
    exit_if(file == NULL, "malloc");
    memset(file, 0, sizeof(*file));

    file->absolute_path = strdup(path);
    exit_if(file->absolute_path == NULL, "strdup");
    file->relative_path = file->absolute_path + strlen(repo->absolute_path) + 1;

    file->type = type;

    // critical section
    file->next = repo->files;
    repo->files = file;
    // critical section end
}

void repository_destroy_file(file_t *file) {
    free(file->absolute_path);
    free(file);
}

void repository_destroy(repository_t *repo) {
    while (repo->files != NULL) {
        file_t *tmp = repo->files;
        repo->files = repo->files->next;
        repository_destroy_file(tmp);
    }
    if (repo->mode == MODE_LOCAL || repo->mode == MODE_DOWNLOAD)
        free(repo->absolute_path);
    if (repo->mode == MODE_REMOTE || repo->mode == MODE_DOWNLOAD)
        free(repo->url);
}
