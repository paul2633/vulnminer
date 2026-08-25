#ifndef GITHUB_H
#define GITHUB_H

#include "config.h"
#include "history.h"
#include "http.h"
#include "jobs.h"

typedef struct {
    config_t *config;
    http_client_t *github_client;
    history_t *history;
    jobs_queue_t *parsing_queue;
} github_global_context_t;

http_client_t *github_client_new(const char *api_key);

char *commit_to_display(unsigned cwe_id, const char *cve_id, const char *repo_name, const char *commit_hash);

void github_process_commit(void *global_context, void *local_context);

#endif
