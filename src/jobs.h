#ifndef JOBS_H
#define JOBS_H

#include <pthread.h>
#include <stdbool.h>

#include "history.h"
#include "http.h"

typedef struct tracker tracker_t;

typedef struct job {
    unsigned cwe_id;
    char *cve_id;
    char *repo_name;
    char *commit_hash;
    struct job *next;
} job_t;

typedef struct {
    job_t *head;
    job_t *tail;
    bool finished;
    pthread_mutex_t lock;
    pthread_cond_t cond;
} jobs_queue_t;

typedef struct {
    jobs_queue_t *queue;
    http_client_t *github_client;
    history_t *history;
} workers_context_t;

jobs_queue_t *jobs_queue_new(void);

void jobs_queue_destroy(jobs_queue_t *queue);

void jobs_queue_finish(jobs_queue_t *queue);

void push_new_job(jobs_queue_t *queue, unsigned cwe_id, char *cve_id, char *repo_name, char *commit_hash);

void *worker(void *arg);

#endif
