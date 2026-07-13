#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

#include "config.h"
#include "exporter.h"
#include "parser.h"
#include "reader.h"
#include "repository.h"
#include "scanner.h"
#include "utils.h"

static void pipeline_sequential(repository_t *repo, config_t *config, FILE *f) {

    parser_t parser;
    parser_init(&parser);

    for (unsigned i = 0; i < repo->file_count; i++) {

        buffer_t source = {0};
        buffer_t json = {0};

        reader_local(repo->ordered_files[i], &source);
        parser_parse(config, &parser, repo->ordered_files[i], &source, &json);

        exporter_export(f, &json, i == repo->file_count - 1);
        
        free(source.data);
        free(json.data);
    }

    parser_destroy(&parser);
}

static void pipeline_parallel_for(repository_t *repo, config_t *config, FILE *f) {

    parser_t parsers[config->threads];
    for (unsigned i = 0; i < config->threads; i++) {
        parser_init(parsers + i);
    }

    buffer_t sources[repo->file_count], jsons[repo->file_count];
    #pragma omp parallel for schedule(dynamic, 1)
    for (unsigned i = 0; i < repo->file_count; i++) {
        int tid = omp_get_thread_num();
        sources[i].data = NULL;
        sources[i].size = 0;
        jsons[i].data = NULL;
        jsons[i].size = 0;

        reader_local(repo->ordered_files[i], sources + i);
        parser_parse(config, parsers + tid, repo->ordered_files[i], sources + i, jsons + i);
        
        free(sources[i].data);
    }

    for (unsigned i = 0; i < repo->file_count; i++) {
        exporter_export(f, jsons+ i, i == repo->file_count - 1);
        free(jsons[i].data);
    }

    for (unsigned i = 0; i < config->threads; i++) {
        parser_destroy(parsers + i);
    }
}

static void pipeline_tasks(repository_t *repo, config_t *config, FILE *f) {

    parser_t parsers[config->threads];
    for (unsigned i = 0; i < config->threads; i++) {
        parser_init(parsers + i);
    }

    buffer_t sources[repo->file_count], jsons[repo->file_count + 1];

    #pragma omp parallel
    #pragma omp single
    {
        for (unsigned i = 0; i < repo->file_count; i++) {
            #pragma omp task firstprivate(i) depend(out: sources[i])
            {
                sources[i] = (buffer_t){0};
                jsons[i] = (buffer_t){0};
                reader_local(repo->ordered_files[i], sources + i);
                parser_parse(config, parsers + omp_get_thread_num(), repo->ordered_files[i], sources + i, jsons + i);
                free(sources[i].data);
            }

            
            #pragma omp task depend(in: sources[i], jsons[i]) depend(out: jsons[i + 1])
            {
                exporter_export(f, jsons + i, i == repo->file_count - 1);
                free(jsons[i].data);
            }
        }
    }

    for (unsigned i = 0; i < config->threads; i++) {
        parser_destroy(parsers + i);
    }
}

int main(int argc, char **argv) {

    config_t config;
    repository_t repo;

    config_init(&config, argc, argv);
    repository_init(&repo, &config);

    if (config.mode == MODE_DOWNLOAD)
        download_repository(config.source, config.commit, repo.name);

    scanner_scan(&repo, &config);

    repository_order_files(&repo);

    FILE *f = exporter_begin(&repo, &config);

    pipeline_tasks(&repo, &config, f);

    exporter_end(f);

    if (config.mode == MODE_DOWNLOAD)
        remove_directory(repo.name);

    repository_destroy(&repo);
    config_destroy(&config);

    return EXIT_SUCCESS;
}
