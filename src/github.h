#ifndef GITHUB_H
#define GITHUB_H

#include "config.h"
#include "http.h"

void github_init_client(http_client_t *client, config_t *config);

#endif
