#ifndef NVD_H
#define NVD_H

#include "config.h"
#include "history.h"
#include "jobs.h"

void nvd_request(const config_t *config, jobs_queue_t *github_queue, history_t *history);

#endif
