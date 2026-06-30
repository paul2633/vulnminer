#include <stdio.h>
#include <stdlib.h>

#include "parser.h"
#include "utils.h"

static void parse_file(file_t *file) {
    FILE *f = fopen(file->path, "r");
    exit_if(f == NULL, "fopen");

    char *line = NULL;
    size_t len = 0;

    while (getline(&line, &len, f) != -1) {
        file->line_count++;
    }

    free(line);

    exit_if(fclose(f) == EOF, "fclose");
}

void parser_parse(repository_t *repo) {
    for (file_t *file = repo->files; file != NULL; file = file->next)
        parse_file(file);
}
