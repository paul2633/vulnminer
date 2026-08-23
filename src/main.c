#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>

#include "config.h"
#include "display.h"
#include "http.h"
#include "nvd.h"

int main(int argc, char **argv) {

    config_t config = {0};
    config_init(&config, argc, argv);

    display_t display = {0};
    display_init(&display);

    http_init();

    nvd_request(&config, &display);

    http_cleanup();

    display_destroy(&display);

    config_destroy(&config);

    return EXIT_SUCCESS;
}
