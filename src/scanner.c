#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "parser.h"
#include "reader.h"
#include "repository.h"
#include "scanner.h"
#include "utils.h"

static void scan_local_directory(repository_t *repo, parser_t *parser, const char *path) {
    DIR *dir = opendir(path);
    exit_if(dir == NULL, "opendir");
    const struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        size_t len = strlen(path) + strlen(entry->d_name) + 2;
        char *sub_path = malloc(len);
        exit_if(sub_path == NULL, "malloc");
        snprintf(sub_path, len, "%s/%s", path, entry->d_name);

        struct stat st;
        exit_if(stat(sub_path, &st) == -1, "stat");

        if (S_ISREG(st.st_mode)) {
            const char *ext = strrchr(entry->d_name, '.');
            if (ext != NULL) {
                file_t *file = NULL;

                if (strcmp(ext, ".c") == 0)
                    file = repository_add_file(repo, sub_path, FILE_C);

                else if (strcmp(ext, ".h") == 0)
                    file = repository_add_file(repo, sub_path, FILE_HEADER);

                if (file != NULL) {
                    // #pragma omp task firstprivate(file)
                    {
                        buffer_t buffer = {0};
                        reader_local(file, &buffer);
                        parser_parse(parser, file, &buffer);
                        free(buffer.data);
                    }
                }
            }
        }

        else if (S_ISDIR(st.st_mode)) {
            scan_local_directory(repo, parser, sub_path);
        }
        free(sub_path);
    }
    exit_if(closedir(dir) == -1, "closedir");
}

static void scan_remote_repository(repository_t *repo, parser_t *parser) {
    (void)repo;
    (void)parser;
}

void scanner_scan(repository_t *repo, parser_t *parser) {
    if (repo->mode == MODE_LOCAL || repo->mode == MODE_DOWNLOAD)
        scan_local_directory(repo, parser, repo->absolute_path);
    else if (repo->mode == MODE_REMOTE)
        scan_remote_repository(repo, parser);
}
