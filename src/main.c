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

    scanner_scan(&repo);

    parser_parse(&repo);

    exporter_export(&repo);

    if (repo.mode == MODE_DOWNLOAD)
        remove_path(repo.name);

    repository_destroy(&repo);

    return EXIT_SUCCESS;
}
