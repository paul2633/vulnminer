#ifndef GITHUB_H
#define GITHUB_H

#include "history.h"
#include "http.h"

typedef struct {
    char *path;
    char *status;
} commit_file_t;

typedef struct {
    char *current_commit_hash;
    char *parent_commit_hash;

    commit_file_t *files;
    unsigned files_count;
} github_commit_t;

http_client_t *github_client_new(const char *api_key);

void github_parse_commit(http_client_t *client, history_t *history, const char *cve_id, const char *repo_name, const char *commit_hash);

#endif
