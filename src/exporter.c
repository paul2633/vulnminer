#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "exporter.h"
#include "repository.h"
#include "utils.h"

static void json_indent(FILE *f, unsigned indent) {
    for (unsigned i = 0; i < indent; i++)
        fputs("    ", f);
}

void exporter_export(repository_t *repo) {
    struct stat st;
    char *output_path = NULL;
    const char *mode = repo->mode == MODE_LOCAL ? "local" : repo->mode == MODE_DOWNLOAD ? "download" : "remote";

    if (stat("../results", &st) == 0 && S_ISDIR(st.st_mode))
        exit_if(asprintf(&output_path, "../results/%s_%s.json", repo->name, mode) == -1, "asprintf");
    else
        exit_if(asprintf(&output_path, "../%s_%s.json", repo->name, mode) == -1, "asprintf");

    FILE *f = fopen(output_path, "w");
    exit_if(f == NULL, "fopen");
    free(output_path);

    fprintf(f, "{\n");

    json_indent(f, 1);
    fprintf(f, "\"repository\": \"%s\",\n", repo->name);

    if (repo->mode == MODE_REMOTE || repo->mode == MODE_DOWNLOAD) {
        json_indent(f, 1);
        fprintf(f, "\"url\": \"%s\",\n", repo->url);
    }

    if (repo->mode == MODE_LOCAL) {
        json_indent(f, 1);
        fprintf(f, "\"absolute_path\": \"%s\",\n", repo->absolute_path);
    }

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

    for (size_t i = 0; i < repo->file_count; i++) {
        const file_t *file = repo->ordered_files[i];

        json_indent(f, 2);
        fprintf(f, "{\n");

        json_indent(f, 3);
        fprintf(f, "\"relative_path\": \"%s\",\n", file->relative_path);

        json_indent(f, 3);
        fprintf(f, "\"type\": \"%s\",\n", file->type == FILE_C ? "c" : "header");

        json_indent(f, 3);
        fprintf(f, "\"line_count\": %zu\n", file->line_count);

        json_indent(f, 2);
        fprintf(f, "}");

        if (i + 1 < repo->file_count)
            fprintf(f, ",");

        fprintf(f, "\n");
    }

    json_indent(f, 1);
    fprintf(f, "]\n");

    fprintf(f, "}\n");

    exit_if(fclose(f) == EOF, "fclose");
}
