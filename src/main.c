#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "exporter.h"
#include "parser.h"
#include "repository.h"
#include "scanner.h"
#include "utils.h"

int main(int argc, char **argv) {

    config_t config;
    repository_t repo;
    parser_t parser;

    config_init(&config, argc, argv);
    repository_init(&repo, &config);
    parser_init(&parser);

    if (config.mode == MODE_DOWNLOAD)
        download_repository(config.source, repo.name);

    scanner_scan(&repo, &config);

    repository_order_files(&repo);

    FILE *f = exporter_begin(&repo, &config);

    exporter_end(f);

    if (config.mode == MODE_DOWNLOAD)
        remove_directory(repo.name);

    parser_destroy(&parser);
    repository_destroy(&repo);
    config_destroy(&config);

    return EXIT_SUCCESS;
}
