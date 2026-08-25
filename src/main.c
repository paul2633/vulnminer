#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "display.h"
#include "github.h"
#include "http.h"
#include "nvd.h"

int main(int argc, char **argv) {

    config_t config = {0};
    config_init(&config, argc, argv);

    display_config(&config);

    display_t display = {0};
    display_init(&display);

    http_client_t github_client = {0};
    github_init_client(&github_client, &config);

    http_init();

    omp_set_num_threads(omp_get_num_procs());

#pragma omp parallel
    {
#pragma omp master
        { nvd_request(&config, &display); }
    }

#pragma omp taskwait

    http_client_destroy(&github_client);
    http_cleanup();

    display_destroy(&display);
    config_destroy(&config);

    return EXIT_SUCCESS;
}
