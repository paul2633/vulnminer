#ifndef JOBS_H
#define JOBS_H

#include <pthread.h>
#include <stdbool.h>

typedef void (*job_function_t)(void *, void *);

typedef struct job {
    void *local_context;
    struct job *next;
} job_t;

typedef struct {
    job_function_t function;
    void *global_context;

    pthread_mutex_t lock;
    pthread_cond_t cond;

    job_t *head;
    job_t *tail;

    bool finished;
} jobs_queue_t;

jobs_queue_t *jobs_queue_new(job_function_t function, void *global_context);

void jobs_queue_destroy(jobs_queue_t *queue);

void jobs_queue_finish(jobs_queue_t *queue);

void push_new_job(jobs_queue_t *queue, void *local_context);

void *worker(void *arg);

#endif
