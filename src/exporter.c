#include <stdio.h>

#include "exporter.h"
#include "repository.h"
#include "utils.h"

void exporter_export(repository_t *repo) {
    FILE *f = fopen("repository.json", "w");
    exit_if(f == NULL, "fopen");

    fprintf(f, "{\n");
    fprintf(f, "  \"repository\": \"%s\",\n", repo->path);
    fprintf(f, "  \"summary\": {\n");
    fprintf(f, "    \"c_files\": %zu,\n", repo->c_file_count);
    fprintf(f, "    \"header_files\": %zu,\n", repo->header_file_count);
    fprintf(f, "    \"total_files\": %zu\n", repo->c_file_count + repo->header_file_count);
    fprintf(f, "  },\n");
    fprintf(f, "  \"files\": [\n");

    file_t *file = repo->files;

    while (file != NULL) {

        fprintf(f,
                "    {\"path\": \"%s\", \"type\": \"%s\"}",
                file->path,
                file->type == FILE_C ? "c" : "header");

        if (file->next != NULL)
            fprintf(f, ",");

        fprintf(f, "\n");

        file = file->next;
    }

    fprintf(f, "  ]\n");
    fprintf(f, "}\n");
    fclose(f);
}