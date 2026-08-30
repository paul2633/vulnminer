#include <stdio.h>
#include <stdlib.h>

#include "github.h"
#include "jobs.h"
#include "utils.h"

jobs_queue_t *jobs_queue_new(void) {
    jobs_queue_t *queue = calloc(1, sizeof(*queue));
    EXIT_IF(queue == NULL, "calloc");

    EXIT_IF(pthread_mutex_init(&queue->lock, NULL) != 0, "pthread_mutex_init");
    EXIT_IF(pthread_cond_init(&queue->cond, NULL) != 0, "pthread_cond_init");

    return queue;
}

void jobs_queue_destroy(jobs_queue_t *queue) {
    pthread_mutex_destroy(&queue->lock);
    pthread_cond_destroy(&queue->cond);
    free(queue);
}

void jobs_queue_finish(jobs_queue_t *queue) {
    pthread_mutex_lock(&queue->lock);

    queue->finished = true;

    pthread_cond_broadcast(&queue->cond);
    pthread_mutex_unlock(&queue->lock);
}

static job_t *job_new(unsigned cwe_id, char *cve_id, char *repo_name, char *commit_hash) {
    job_t *job = calloc(1, sizeof(*job));
    EXIT_IF(job == NULL, "calloc");

    job->cwe_id = cwe_id;
    job->cve_id = cve_id;
    job->repo_name = repo_name;
    job->commit_hash = commit_hash;

    return job;
}

static void job_destroy(job_t *job) {
    free(job->cve_id);
    free(job->repo_name);
    free(job->commit_hash);
    free(job);
}

void push_new_job(jobs_queue_t *queue, unsigned cwe_id, char *cve_id, char *repo_name, char *commit_hash) {
    job_t *job = job_new(cwe_id, cve_id, repo_name, commit_hash);
    pthread_mutex_lock(&queue->lock);

    if (queue->tail == NULL)
        queue->head = job;
    else
        queue->tail->next = job;
    queue->tail = job;

    pthread_cond_signal(&queue->cond);
    pthread_mutex_unlock(&queue->lock);
}

static job_t *pop_job(jobs_queue_t *queue) {
    pthread_mutex_lock(&queue->lock);

    while (queue->head == NULL && !queue->finished)
        pthread_cond_wait(&queue->cond, &queue->lock);

    if (queue->head == NULL) {
        pthread_mutex_unlock(&queue->lock);
        return NULL;
    }

    job_t *job = queue->head;
    queue->head = job->next;

    if (queue->head == NULL)
        queue->tail = NULL;

    pthread_mutex_unlock(&queue->lock);
    return job;
}

void *worker(void *arg) {
    workers_context_t *context = arg;

    while (true) {
        job_t *job = pop_job(context->queue);

        if (job == NULL)
            break;

        github_parse_commit(context->github_client, context->history, job->cwe_id, job->cve_id, job->repo_name, job->commit_hash);

        job_destroy(job);
    }

    return NULL;
}
