#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "arguments.h"
#include "repository.h"
#include "utils.h"

void arguments_parse(int argc, char *argv[], repository_t *repo) {
    opterr = 0;

    int opt;
    while ((opt = getopt_long(argc, argv, "l:d:r", NULL, NULL)) != -1) {
        switch (opt) {
        case 'l': {
            exit_if(repo->mode != MODE_NONE, "Incompatible options");
            repo->mode = MODE_LOCAL;

            repo->absolute_path = realpath(optarg, NULL);
            exit_if(repo->absolute_path == NULL, "Specified path not found");

            char *slash = strrchr(repo->absolute_path, '/');
            exit_if(slash == NULL, "strrchr");
            repo->name = slash + 1;
            break;
        }

        case 'd': {
            exit_if(repo->mode != MODE_NONE, "Incompatible options");
            repo->mode = MODE_DOWNLOAD;

            repo->url = strdup(optarg);
            exit_if(repo->url == NULL, "strdup");

            char *slash = strrchr(repo->url, '/');
            exit_if(slash == NULL, "strrchr");
            repo->name = slash + 1;

            download_repository(repo);
            repo->absolute_path = realpath(repo->name, NULL);
            exit_if(repo->absolute_path == NULL, "realpath");
            break;
        }

        case 'r': {
            exit_if(repo->mode != MODE_NONE, "Incompatible options");
            repo->mode = MODE_REMOTE;

            repo->url = strdup(optarg);
            exit_if(repo->url == NULL, "strdup");

            char *slash = strrchr(repo->url, '/');
            exit_if(slash == NULL, "strrchr");
            repo->name = slash + 1;
            break;
        }

        default: {
            exit_if(1, "Unknown option or missing argument");
        }
        }
    }

    if (repo->mode == MODE_NONE)
        exit_if(1, "One of -l, -d or -r is required");
}
