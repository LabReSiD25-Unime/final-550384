#ifndef WORKERS_H
#define WORKERS_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#include "requests_queue.h"



typedef struct {
    pthread_t *threads;
    int num_threads;
    request_queue_t *queue;
    bool shutdown;
    void* (*process_function)(void *data);
} worker_pool_t;

extern worker_pool_t *worker_pool;

void worker_pool_destroy(worker_pool_t *pool);
void* worker_thread(void *arg);
worker_pool_t* worker_pool_init(int num_threads, void* (*process_func)(void*));

#endif