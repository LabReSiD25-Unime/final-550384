#include "workers.h"
#include "requests_queue.h"

// Inizializza il pool di worker thread
worker_pool_t* worker_pool_init(int num_threads, void* (*process_func)(void*)) {
    if (num_threads <= 0 || !process_func) return NULL;

    worker_pool_t *pool = malloc(sizeof(worker_pool_t));
    if (!pool) return NULL;

    pool->threads = malloc(sizeof(pthread_t) * num_threads);
    if (!pool->threads) {
        free(pool);
        return NULL;
    }

    pool->queue = createQueue(20);
    if (!pool->queue) {
        free(pool->threads);
        free(pool);
        return NULL;
    }

    pool->num_threads = num_threads;
    pool->shutdown = false;
    pool->process_function = process_func;

    // Crea i thread worker
    for (int i = 0; i < num_threads; i++) {
        if (pthread_create(&pool->threads[i], NULL, worker_thread, pool) != 0) {
            // Errore nella creazione del thread
            pool->num_threads = i; // Aggiorna il numero di thread creati
            worker_pool_destroy(pool);
            return NULL;
        }
    }

    return pool;
}

void* worker_thread(void *arg) {
    sleep(3);
    http_request_t * incoming_request = malloc(sizeof(http_request_t));
    
    if(dequeue(worker_pool->queue, incoming_request)){
        print_http_request(incoming_request);
    }

    printQueue(worker_pool->queue);
    return NULL;
}


void worker_pool_destroy(worker_pool_t *pool) {
    if (!pool) return;

    // Segnala shutdown
    pool->shutdown = true;

    pthread_mutex_lock(&pool->queue->mutex);
    pool->queue->shutdownFlag = true;
    pthread_cond_broadcast(&pool->queue->notEmpty);
    pthread_mutex_unlock(&pool->queue->mutex);

    // Aspetta che tutti i thread terminino
    for (int i = 0; i < pool->num_threads; i++) {
        pthread_join(pool->threads[i], NULL);
    }

    clearQueue(pool->queue);

    // Distruggi mutex e condition variable
    pthread_mutex_destroy(&pool->queue->mutex);
    pthread_cond_destroy(&pool->queue->notEmpty);

    free(pool->queue);
    free(pool->threads);
    free(pool);
}


/*         http_response_t *json_response = create_http_response();
        if (json_response) {
            set_response_status(json_response, HTTP_OK);
            set_response_json(json_response, "{\"status\":\"success\",\"message\":\"Utente creato\"}");
            add_response_header(json_response, "X-Custom-Header", "MyValue");
            
            print_http_response(json_response);
            
            const char *response_string = get_response_string(json_response);
            if (response_string) {
                printf("\n--- Risposta raw ---\n");
                printf("%s\n", response_string);
            }
            
            send(client_fd,response_string,strlen(response_string),0);
            free_http_response(json_response);
        } 
 */