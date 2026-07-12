#include <string.h>

#include "config.h"
#include "repository.h"
#include "utils.h"

void repository_init(repository_t *repo, config_t *config) {
    memset(repo, 0, sizeof(*repo));

    char *name = strrchr(config->source, '/');
    repo->name = name == NULL ? config->source : name + 1;
}

void repository_add_file(repository_t *repo, char *path, int offset) {
    file_t *file = malloc(sizeof(file_t));
    exit_if(file == NULL, __func__, "malloc");
    memset(file, 0, sizeof(*file));

    file->absolute_path = path;
    file->relative_path = path + offset + 1;
    file->name = strrchr(path, '/') + 1;

    file->next = repo->files;
    repo->files = file;
    repo->file_count++;
}

static int compare_files(const void *a, const void *b) {
    const file_t *fa = *(const file_t *const *)a;
    const file_t *fb = *(const file_t *const *)b;

    return strcmp(fa->relative_path, fb->relative_path);
}

void repository_order_files(repository_t *repo) {
    repo->ordered_files = malloc(repo->file_count * sizeof(*repo->ordered_files));
    exit_if(repo->ordered_files == NULL, __func__, "malloc");

    size_t i = 0;
    for (file_t *f = repo->files; f != NULL; f = f->next)
        repo->ordered_files[i++] = f;

    qsort(repo->ordered_files, repo->file_count, sizeof(*repo->ordered_files), compare_files);
}

void repository_destroy(repository_t *repo) {
    free(repo->ordered_files);
    while (repo->files != NULL) {
        file_t *tmp = repo->files;
        repo->files = repo->files->next;
        free(tmp->absolute_path);
        free(tmp);
    }
}
