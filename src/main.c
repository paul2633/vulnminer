#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "github.h"
#include "history.h"
#include "http.h"
#include "nvd.h"

int main(int argc, char **argv) {

    config_t *config = config_new(argc, argv);

    history_t *history = history_new();

    display_config(config);

    printf("\n");

    display_history(history);

    http_init();

    http_client_t *github_client = github_client_new(config->github_api_key);

    omp_set_num_threads(omp_get_num_procs());

#pragma omp parallel
    {
#pragma omp master
        { nvd_request(config, history, github_client); }
    }

#pragma omp taskwait

    http_client_destroy(github_client);

    http_cleanup();

    history_destroy(history);

    config_destroy(config);

    return EXIT_SUCCESS;
}
