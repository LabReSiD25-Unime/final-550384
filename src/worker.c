#include "workers.h"
#include "requests_queue.h"
#include "server_utils.h"
#include "book.h"

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
        int thread_id = i;
        if (pthread_create(&pool->threads[i], NULL, worker_thread, &thread_id) != 0) {
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

    redisContext *c = connect_redis();
    if (c == NULL) {
        return;
    }

    client_request_node_t * incoming_request = malloc(sizeof(client_request_node_t));
    
    while (1){
        if(dequeue_node(worker_pool->queue, incoming_request)){

            http_response_t *response = process_rest_request(&incoming_request->request);
            
            if (response) {
                
                const char *response_string = get_response_string(response);
                if (response_string) {
                    printf("\n--- Risposta raw ---\n");
                    printf("%s\n", response_string);
                }
                
                send(incoming_request->client_fd,response_string,strlen(response_string),0);
                free_http_response(response);
            } 
        }

        printQueue(worker_pool->queue);
    }
    

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

void crud_create(const http_request_t *request, http_response_t *response){

    if(strncmp(request->path, "/add/book", 10) != 0) {
        printf("Endpoint non supportato: %s\n", request->path);
        set_response_status(response, HTTP_NOT_FOUND);
        set_response_json(response, "{\"error\": \"Endpoint non trovato\"}");
        return;
    }

    

    set_response_status(response, HTTP_OK);
    set_response_json(response, "{\"status\":\"success\",\"message\":\"Utente creato\"}");
    add_response_header(response, "X-Custom-Header", "MyValue");


}

http_response_t* process_rest_request(http_request_t *request){

    http_response_t *response = create_http_response();

    // Routing basato sul metodo HTTP
    switch (request->method) {
        case HTTP_POST:
            crud_create(request, response);
            break;
            
        case HTTP_GET:
            printf("GET\n");
            break;
            
        case HTTP_PUT:
        case HTTP_PATCH:
            
            break;
            
        case HTTP_DELETE:
            printf("Delete\n");
            break;
            
        default:
            printf("Metodo HTTP non supportato: %s\n", request->method_str);
            set_response_status(response, HTTP_METHOD_NOT_ALLOWED);
            set_response_json(response, "{\"error\": \"Metodo non supportato\"}");
            add_response_header(response, "Allow", "GET, POST, PUT, PATCH, DELETE");
            break;
    }

    return response;
}