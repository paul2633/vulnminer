#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "config.h"
#include "github.h"
#include "history.h"
#include "http.h"
#include "jobs.h"
#include "nvd.h"
#include "utils.h"

#define CORES_NUMBER sysconf(_SC_NPROCESSORS_ONLN)

int main(int argc, char **argv) {

    config_t *config = config_new(argc, argv);

    display_config(config);

    printf("\n");

    history_t *history = history_new();

    jobs_queue_t *queue = jobs_queue_new();

    http_client_t *github_client = github_client_new(config->github_api_key);

    workers_context_t context = {queue, github_client, history};

    pthread_t threads[CORES_NUMBER];
    for (int i = 0; i < CORES_NUMBER; i++)
        EXIT_IF(pthread_create(threads + i, NULL, worker, &context) != 0, "pthread_create");

    http_init();

    nvd_request(config, queue, history);

    jobs_queue_finish(queue);

    for (int i = 0; i < CORES_NUMBER; i++) {
        pthread_join(threads[i], NULL);
    }

    http_client_destroy(github_client);

    http_cleanup();

    history_destroy(history);

    jobs_queue_destroy(queue);

    config_destroy(config);

    return EXIT_SUCCESS;
}
