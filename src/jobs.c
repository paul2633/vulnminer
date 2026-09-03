#include <stdio.h>
#include <stdlib.h>

#include "jobs.h"
#include "utils.h"

jobs_queue_t *jobs_queue_new(job_function_t function, void *global_context) {
    jobs_queue_t *queue = calloc(1, sizeof(*queue));
    EXIT_IF(queue == NULL, "calloc");

    queue->function = function;
    queue->global_context = global_context;

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

void push_new_job(jobs_queue_t *queue, void *local_context) {
    job_t *job = calloc(1, sizeof(*job));
    EXIT_IF(job == NULL, "calloc");
    job->local_context = local_context;

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
    jobs_queue_t *queue = arg;

    while (true) {
        job_t *job = pop_job(queue);
        if (job == NULL)
            break;

        queue->function(queue->global_context, job->local_context);
        free(job);
    }

    return NULL;
}
