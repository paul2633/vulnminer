#ifndef NVD_H
#define NVD_H

#include "config.h"
#include "history.h"
#include "http.h"

void nvd_request(const config_t *config, history_t *history, http_client_t *github_client);

#endif
