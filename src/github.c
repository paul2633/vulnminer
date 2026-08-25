#include <stdio.h>
#include <stdlib.h>

#include "github.h"
#include "config.h"
#include "http.h"
#include "utils.h"

void github_init_client(http_client_t *client, config_t *config) {
    http_client_init(client);

    if (config->github_api_key != NULL) {
        char *key = NULL;

        EXIT_IF(asprintf(&key, "Authorization: Bearer %s", config->github_api_key) == -1, "asprintf");

        http_client_add_header(client, key);
        free(key);
    }

    http_client_add_header(client, "Accept: application/vnd.github+json");
    http_client_add_header(client, "User-Agent: repo-analyzer");
}
