#include <stdio.h>

#include "exporter.h"
#include "repository.h"
#include "utils.h"

static void json_indent(FILE *f, unsigned indent) {
    for (unsigned i = 0; i < indent; i++)
        fputs("    ", f);
}

void exporter_export(repository_t *repo) {
    FILE *f = fopen("../results/repository.json", "w");
    exit_if(f == NULL, "fopen");

    fprintf(f, "{\n");

    json_indent(f, 1);
    fprintf(f, "\"repository\": \"%s\",\n", repo->path);

    json_indent(f, 1);
    fprintf(f, "\"summary\": {\n");

    json_indent(f, 2);
    fprintf(f, "\"c_files\": %zu,\n", repo->c_file_count);

    json_indent(f, 2);
    fprintf(f, "\"header_files\": %zu\n", repo->header_file_count);

    json_indent(f, 1);
    fprintf(f, "},\n");

    json_indent(f, 1);
    fprintf(f, "\"files\": [\n");

    file_t *file = repo->files;

    while (file != NULL) {
        json_indent(f, 2);
        fprintf(f, "{\n");

        json_indent(f, 3);
        fprintf(f, "\"path\": \"%s\",\n", file->path);

        json_indent(f, 3);
        fprintf(f, "\"type\": \"%s\"\n", file->type == FILE_C ? "c" : "header");

        json_indent(f, 2);
        fprintf(f, "}");

        if (file->next != NULL)
            fprintf(f, ",");

        fprintf(f, "\n");

        file = file->next;
    }

    json_indent(f, 1);
    fprintf(f, "]\n");

    fprintf(f, "}\n");

    exit_if(fclose(f) == EOF, "fclose");
}
