#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "config.h"
#include "exporter.h"
#include "reader.h"
#include "repository.h"
#include "utils.h"

FILE *exporter_begin(repository_t *repo, config_t *config) {
    char *output_path = NULL;
    
    exit_if(asprintf(&output_path, "%s/%s_%s_%s.json", config->json_path, repo->name, mode_to_string(config->mode), granularity_to_string(config->granularity)) == -1, __func__, "asprintf");
    free(config->json_path);
    config->json_path = output_path;
    FILE *f = fopen(config->json_path, "w");
    exit_if(f == NULL, __func__, "fopen");
    
    json_write(f, 0, "{\n");
    json_write(f, 1, "\"repository\": \"%s\",\n", repo->name);
    json_write(f, 1, "\"mode\": \"%s\",\n", mode_to_string(config->mode));
    json_write(f, 1, "\"source\": \"%s\",\n", config->source);
    if (config->mode == MODE_DOWNLOAD || config->mode == MODE_REMOTE)
        json_write(f, 1, "\"commit\": \"%s\",\n", config->commit == NULL ? "HEAD" : config->commit);

    json_write(f, 1, "\"excluded directories\": [", config->source);
    if (config->exclude != NULL) {
        for (size_t i = 0; config->exclude[i] != NULL; i++) {
            json_write(f, 0, "\"%s\"", config->exclude[i]);
            if (config->exclude[i + 1] != NULL)
                json_write(f, 0, ", ", config->exclude[i]);
        }
    }
    json_write(f, 0, "],\n");

    //json_write(f, 1, "\"file_count\": %zu,\n", repo->file_count);
    json_write(f, 1, "\"files\": [\n");

    return f;
}

void exporter_export(FILE *f, const buffer_t *buffer, bool last) {
    exit_if(fwrite(buffer->data, 1, buffer->size, f) != buffer->size, __func__, "fwrite");
    if (!last)
        exit_if(fputs(",\n", f) == EOF, __func__, "fputs");
    else
        exit_if(fputs("\n", f) == EOF, __func__, "fputs");
}

void exporter_end(FILE *f) {
    json_write(f, 1, "]\n");
    json_write(f, 0, "}\n");

    exit_if(fclose(f) == EOF, __func__, "fclose");
}
