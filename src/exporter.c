#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "exporter.h"
#include "repository.h"
#include "utils.h"

static void json_indent(FILE *f, unsigned indent) {
    for (unsigned i = 0; i < indent; i++)
        fputs("    ", f);
}

FILE *exporter_begin(repository_t *repo, config_t *config) {
    char *output_path = NULL;
    const char *mode = config->mode == MODE_LOCAL ? "local" : config->mode == MODE_DOWNLOAD ? "download" : "remote";

    exit_if(asprintf(&output_path, "../results/%s_%s.json", repo->name, mode) == -1, __func__, "asprintf");
    FILE *f = fopen(output_path, "w");
    exit_if(f == NULL, __func__, "fopen");
    free(output_path);

    fprintf(f, "{\n");

    json_indent(f, 1);
    fprintf(f, "\"repository\": \"%s\",\n", repo->name);

    json_indent(f, 1);
    fprintf(f, "\"mode\": \"%s\",\n", mode);

    json_indent(f, 1);
    fprintf(f, "\"source\": \"%s\",\n", config->source);

    json_indent(f, 1);
    fprintf(f, "\"file_count\": %zu,\n", repo->file_count);

    json_indent(f, 1);
    fprintf(f, "\"files\": [\n");

    return f;
}

void exporter_end(FILE *f) {
    json_indent(f, 1);
    fprintf(f, "]\n");

    fprintf(f, "}\n");

    exit_if(fclose(f) == EOF, __func__, "fclose");
}

/*
void exporter_export(repository_t *repo, config_t *config) {
    char *output_path = NULL;
    const char *mode = config->mode == MODE_LOCAL ? "local" : config->mode == MODE_DOWNLOAD ? "download" : "remote";

    exit_if(asprintf(&output_path, "../results/%s_%s.json", repo->name, mode) == -1, __func__, "asprintf");
    FILE *f = fopen(output_path, "w");
    exit_if(f == NULL, __func__, "fopen");
    free(output_path);

    fprintf(f, "{\n");

    json_indent(f, 1);
    fprintf(f, "\"repository\": \"%s\",\n", repo->name);

    json_indent(f, 1);
    fprintf(f, "\"mode\": \"%s\",\n", mode);

    json_indent(f, 1);
    fprintf(f, "\"source\": \"%s\",\n", config->source);

    json_indent(f, 1);
    fprintf(f, "\"file_count\": \"%zu\",\n", repo->file_count);

    json_indent(f, 1);
    fprintf(f, "\"files\": [\n");

    for (size_t i = 0; i < repo->file_count; i++) {
        const file_t *file = repo->ordered_files[i];

        json_indent(f, 2);
        fprintf(f, "{\n");

        json_indent(f, 3);
        fprintf(f, "\"name\": \"%s\",\n", file->name);

        json_indent(f, 3);
        fprintf(f, "\"path\": \"%s\",\n", file->relative_path);

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

    exit_if(fclose(f) == EOF, __func__, "fclose");
}
*/
