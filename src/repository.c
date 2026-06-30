#include <string.h>

#include "repository.h"
#include "utils.h"

void repository_init(repository_t *repo, char *path) {
    memset(repo, 0, sizeof(*repo));
    repo->path = strdup(path);
    exit_if(repo->path == NULL, "strdup");
}

void repository_destroy(repository_t *repo) {
    free(repo->path);
}