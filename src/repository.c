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
    if (file->type == FILE_C)
        repo->c_file_count++;
    else if (file->type == FILE_HEADER)
        repo->header_file_count++;
    // critical section end
}

static int compare_files(const void *a, const void *b) {
    const file_t *fa = *(const file_t *const *)a;
    const file_t *fb = *(const file_t *const *)b;

    return strcmp(fa->relative_path, fb->relative_path);
}

void repository_order_files(repository_t *repo) {
    repo->file_count = repo->c_file_count + repo->header_file_count;

    repo->ordered_files = malloc(repo->file_count * sizeof(*repo->ordered_files));
    exit_if(repo->ordered_files == NULL, "malloc");

    size_t i = 0;
    for (file_t *f = repo->files; f != NULL; f = f->next)
        repo->ordered_files[i++] = f;

    qsort(repo->ordered_files, repo->file_count, sizeof(*repo->ordered_files), compare_files);
}

void repository_destroy_file(file_t *file) {
    free(file->absolute_path);
    free(file);
}

void repository_destroy(repository_t *repo) {
    if (repo->mode == MODE_LOCAL || repo->mode == MODE_DOWNLOAD)
        free(repo->absolute_path);
    if (repo->mode == MODE_REMOTE || repo->mode == MODE_DOWNLOAD)
        free(repo->url);
    while (repo->files != NULL) {
        file_t *tmp = repo->files;
        repo->files = repo->files->next;
        repository_destroy_file(tmp);
    }
    free(repo->ordered_files);
}
