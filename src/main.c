#include <stdlib.h>

#include "arguments.h"
#include "exporter.h"
#include "parser.h"
#include "repository.h"
#include "scanner.h"

int main(int argc, char **argv) {

    repository_t repo;

    repository_init(&repo);

    arguments_parse(argc, argv, &repo);

    parser_t parser;

    parser_init(&parser);

    scanner_scan(&repo, &parser);

    parser_destroy(&parser);

    repository_order_files(&repo);

    exporter_export(&repo);

    repository_destroy(&repo);

    return EXIT_SUCCESS;
}
