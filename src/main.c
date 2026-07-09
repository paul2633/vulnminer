#include <stdlib.h>

#include "arguments.h"
#include "exporter.h"
#include "parser.h"
#include "repository.h"
#include "scanner.h"
#include "utils.h"

int main(int argc, char **argv) {

    repository_t repo;

    repository_init(&repo);

    arguments_parse(argc, argv, &repo);

    if (repo.mode == MODE_DOWNLOAD) {
        download_repository(&repo);
        repo.absolute_path = realpath(repo.name, NULL);
        exit_if(repo.absolute_path == NULL, "realpath");
    }

    scanner_scan(&repo);

    parser_parse(&repo);

    repository_order_files(&repo);

    exporter_export(&repo);

    if (repo.mode == MODE_DOWNLOAD)
        remove_directory(repo.name);

    repository_destroy(&repo);

    return EXIT_SUCCESS;
}
