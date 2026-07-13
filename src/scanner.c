#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "config.h"
#include "repository.h"
#include "scanner.h"
#include "utils.h"

static bool is_excluded(const config_t *config, const char *name) {
    if (config->exclude == NULL)
        return false;

    for (size_t i = 0; config->exclude[i] != NULL; i++)
        if (strcmp(name, config->exclude[i]) == 0)
            return true;

    return false;
}

static void scan_local_directory(repository_t *repo, const config_t *config, const char *path, int offset_relative_path) {
    DIR *dir = opendir(path);
    exit_if(dir == NULL, __func__, "opendir");

    const struct dirent *entry;
    int offset_name = strlen(path) + 1;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        if (is_excluded(config, entry->d_name)) {
            continue;
        }

        char *sub_path = NULL;
        exit_if(asprintf(&sub_path, "%s/%s", path, entry->d_name) == -1, __func__, "asprintf");

        struct stat st;
        exit_if(stat(sub_path, &st) == -1, __func__, "stat");

        if (!S_ISREG(st.st_mode)) {
            if (S_ISDIR(st.st_mode))
                scan_local_directory(repo, config, sub_path, offset_relative_path);
            free(sub_path);
            continue;
        }

        const char *ext = strrchr(entry->d_name, '.');
        if (ext == NULL || (strcmp(ext, ".c") != 0 && strcmp(ext, ".h") != 0)) {
            free(sub_path);
            continue;
        }

        repository_add_file(repo, sub_path, offset_relative_path, offset_name);
    }
    exit_if(closedir(dir) == -1, __func__, "closedir");
}

static void scan_remote_repository(repository_t *repo) { (void)repo; }

void scanner_scan(repository_t *repo, const config_t *config) {
    if (config->mode == MODE_LOCAL) {
        scan_local_directory(repo, config, config->source, strlen(config->source) + 1);
    }

    else if (config->mode == MODE_DOWNLOAD) {
        char *path = realpath(repo->name, NULL);
        exit_if(path == NULL, __func__, "realpath");
        scan_local_directory(repo, config, path, strlen(path) + 1);
        free(path);
    }

    else if (config->mode == MODE_REMOTE) {
        scan_remote_repository(repo);
    }
}
