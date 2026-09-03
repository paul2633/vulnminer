#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "github/github.h"
#include "history.h"
#include "http.h"
#include "jobs.h"
#include "nvd/nvd.h"
#include "tree_sitter/parser.h"
#include "utils.h"

#define GITHUB_THREADS 1
#define PARSING_THREADS 3

int main(int argc, char **argv) {

    config_t *config = config_new(argc, argv);

    display_config(config);

    printf("\n");

    history_t *history = history_new();

    http_init();

    /*--- PARSING ---*/

    parser_global_context_t parser_global_context = {config, history};

    jobs_queue_t *parsing_queue = jobs_queue_new(parser_export_commit, &parser_global_context);

    pthread_t parsing_threads[PARSING_THREADS];
    for (int i = 0; i < PARSING_THREADS; i++)
        EXIT_IF(pthread_create(parsing_threads + i, NULL, worker, parsing_queue) != 0, "pthread_create");

    /*--- GITHUB ---*/

    http_client_t *github_client = github_client_new(config->github_api_key);

    github_global_context_t github_global_context = {config, github_client, history, parsing_queue};

    jobs_queue_t *github_queue = jobs_queue_new(github_process_commit, &github_global_context);

    pthread_t github_threads[GITHUB_THREADS];
    for (int i = 0; i < GITHUB_THREADS; i++)
        EXIT_IF(pthread_create(github_threads + i, NULL, worker, github_queue) != 0, "pthread_create");

    /* MAIN WORK */

    nvd_request(config, github_queue, history);

    jobs_queue_finish(github_queue);

    for (int i = 0; i < GITHUB_THREADS; i++)
        pthread_join(github_threads[i], NULL);

    jobs_queue_finish(parsing_queue);

    for (int i = 0; i < PARSING_THREADS; i++)
        pthread_join(parsing_threads[i], NULL);

    /* CLEANUP */

    http_client_destroy(github_client);

    http_cleanup();

    jobs_queue_destroy(github_queue);

    jobs_queue_destroy(parsing_queue);

    history_destroy(history);

    config_destroy(config);

    return EXIT_SUCCESS;
}
