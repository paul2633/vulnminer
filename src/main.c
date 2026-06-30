#include <stdio.h>
#include <stdlib.h>

#include "exporter.h"
#include "parser.h"
#include "repository.h"
#include "scanner.h"

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <repository>\n", argv[0]);
        return EXIT_FAILURE;
    }

    repository_t repo;

    repository_init(&repo, argv[1]);

    scanner_scan(&repo);

    parser_parse(&repo);

    exporter_export(&repo);

    repository_destroy(&repo);

    return EXIT_SUCCESS;
}
