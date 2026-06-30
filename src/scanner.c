#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>

#include "scanner.h"
#include "repository.h"
#include "utils.h"

static void scan_directory(repository_t *repo, const char *path) {
    DIR *dir = opendir(path);
    exit_if(dir == NULL, "opendir");
    struct dirent *entry;

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
                if (strcmp(ext, ".c") == 0) {
                    repo->c_file_count++;
                }
                
                else if (strcmp(ext, ".h") == 0) {
                    repo->header_file_count++;
                }
            }
        }
        
        else if (S_ISDIR(st.st_mode)) {
            scan_directory(repo, sub_path);
        }
        free(sub_path);
    }
    exit_if(closedir(dir) == -1, "closedir");
}

void scanner_scan(repository_t *repo) {
    scan_directory(repo, repo->path);
}