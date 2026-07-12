#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "config.h"
#include "repository.h"
#include "scanner.h"
#include "utils.h"

static void scan_local_directory(repository_t *repo, const char *path, int offset) {
    DIR *dir = opendir(path);
    exit_if(dir == NULL, __func__, "opendir");
    const struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char *sub_path = NULL;
        exit_if(asprintf(&sub_path, "%s/%s", path, entry->d_name) == -1, __func__, "asprintf");

        struct stat st;
        exit_if(stat(sub_path, &st) == -1, __func__, "stat");

        if (!S_ISREG(st.st_mode)) {
            if (S_ISDIR(st.st_mode))
                scan_local_directory(repo, sub_path, offset);
            free(sub_path);
            continue;
        }

        const char *ext = strrchr(entry->d_name, '.');
        if (ext == NULL || (strcmp(ext, ".c") != 0 && strcmp(ext, ".h") != 0)) {
            free(sub_path);
            continue;
        }

        repository_add_file(repo, sub_path, offset);
    }
    exit_if(closedir(dir) == -1, __func__, "closedir");
}

static void scan_remote_repository(repository_t *repo) { (void)repo; }

void scanner_scan(repository_t *repo, config_t *config) {
    if (config->mode == MODE_REMOTE) {
        scan_remote_repository(repo);
        return;
    }

    const char *source = config->mode == MODE_LOCAL ? config->source : repo->name;
    char *path = realpath(source, NULL);
    exit_if(path == NULL, __func__, "realpath");

    scan_local_directory(repo, path, strlen(path));
    free(path);
}
