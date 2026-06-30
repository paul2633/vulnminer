#include <stdio.h>
#include <stdlib.h>

#include "repository.h"
#include "scanner.h"
#include "exporter.h"

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <repository>\n", argv[0]);
        return EXIT_FAILURE;
    }

    repository_t repo;

    repository_init(&repo, argv[1]);

    scanner_scan(&repo);
    
    //parser_parse(&repo);

    exporter_export(&repo);

    //printf("path: TODO\n");
    //printf("scan_date: TODO\n");
    //printf("c_files: %ld\n", repo.file_count);
    //printf("header_files: %ld\n", repo.header_count);
    //printf("total_files: TODO\n");

    repository_destroy(&repo);

    return EXIT_SUCCESS;
}