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

    redisContext *c = get_redis_connection();
    if (c == NULL) {
        return;
    }

    client_request_node_t * incoming_request = malloc(sizeof(client_request_node_t));
    
    while (1){
        if(dequeue_node(worker_pool->queue, incoming_request)){

            http_response_t *response = process_rest_request(&incoming_request->request, c);
            
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

void crud_create(const http_request_t *request, http_response_t *response, redisContext *c){

    if(strncmp(request->path, "/add/book", 10) != 0) {
        printf("Endpoint non supportato: %s\n", request->path);
        set_response_status(response, HTTP_NOT_FOUND);
        set_response_json(response, "{\"error\": \"Endpoint non trovato\"}");
        return;
    }

    Book new_book;

    if (parse_book_json(request->body, &new_book)) {
        printf("Parsing completato con successo!\n\n");
    } else {
        printf("Errore durante il parsing del JSON\n");

    }

    
    if(save_book(c, &new_book) == -1){
        set_response_status(response, HTTP_INTERNAL_SERVER_ERROR);
        set_response_json(response, "{\"error\": \ Errore interno al server...riprova e sarai più fortunato... \"}");
        add_response_header(response, "X-Custom-Header", "MyValue");
    }
    

    

    set_response_status(response, HTTP_OK);
    set_response_json(response, request->body);
    add_response_header(response, "X-Custom-Header", "MyValue");


}

void crud_read(const http_request_t *request, http_response_t *response, redisContext *c ){
    if(strncmp(request->path, "/get/books", 11) != 0) {
        printf("Endpoint non supportato: %s\n", request->path);
        set_response_status(response, HTTP_NOT_FOUND);
        set_response_json(response, "{\"error\": \"Endpoint non trovato\"}");
        return;
    }

    Book new_book;

    if (parse_book_json(request->body, &new_book)) {
        printf("Parsing completato con successo!\n\n");
    } else {
        printf("Errore durante il parsing del JSON\n");

    }

    Book *loaded_book = load_book(c, new_book.id) ;
    
    if(loaded_book== NULL){
        set_response_status(response, HTTP_INTERNAL_SERVER_ERROR);
        set_response_json(response, "{\"error\": \ Errore interno al server...riprova e sarai più fortunato... \"}");
        add_response_header(response, "X-Custom-Header", "MyValue");
    }
    
    char resp_body[256];
    snprintf(resp_body, 256,
        "{\n"
        "    \"id_book\": %d,\n"
        "    \"title\": \"%s\",\n"
        "    \"author\": \"%s\",\n"
        "    \"price\": %.2f\n"
        "}",
        loaded_book->id, loaded_book->title, loaded_book->author, loaded_book->price);

    
    set_response_status(response, HTTP_OK);
    set_response_json(response, resp_body);
    add_response_header(response, "X-Custom-Header", "MyValue");


}

void crud_delete(const http_request_t *request, http_response_t *response, redisContext *c ){
    if(strncmp(request->path, "/delete/book", 13) != 0) {
        printf("Endpoint non supportato: %s\n", request->path);
        set_response_status(response, HTTP_NOT_FOUND);
        set_response_json(response, "{\"error\": \"Endpoint non trovato\"}");
        return;
    }

    Book new_book;

    if (parse_book_json(request->body, &new_book)) {
        printf("Parsing completato con successo!\n\n");
    } else {
        printf("Errore durante il parsing del JSON\n");

    }

    
    
    if(delete_book(c,new_book.id) < 0){
        set_response_status(response, HTTP_INTERNAL_SERVER_ERROR);
        set_response_json(response, "{\"error\": \ Errore interno al server...riprova e sarai più fortunato... \"}");
        add_response_header(response, "X-Custom-Header", "MyValue");
    }
    
    char resp_body[256];
    snprintf(resp_body, 256,
        "{\n"
        "    \"id_book\": %d,\n"
        "    \"title\": \"%s\",\n"
        "    \"author\": \"%s\",\n"
        "    \"price\": %.2f\n"
        "}",
        new_book.id, new_book.title, new_book.author, new_book.price);

    
    set_response_status(response, HTTP_OK);
    set_response_json(response, resp_body);
    add_response_header(response, "X-Custom-Header", "MyValue");

}

void crud_update(const http_request_t *request, http_response_t *response, redisContext *c ){

     if(strncmp(request->path, "/update/book", 13) != 0) {
        printf("Endpoint non supportato: %s\n", request->path);
        set_response_status(response, HTTP_NOT_FOUND);
        set_response_json(response, "{\"error\": \"Endpoint non trovato\"}");
        return;
    }

    Book new_book;

    if (parse_book_json(request->body, &new_book)) {
        printf("Parsing completato con successo!\n\n");
    } else {
        printf("Errore durante il parsing del JSON\n");

    }

    
    
    if(update_book_price(c,new_book.id, new_book.price) < 0){
        set_response_status(response, HTTP_INTERNAL_SERVER_ERROR);
        set_response_json(response, "{\"error\": \ Errore interno al server...riprova e sarai più fortunato... \"}");
        add_response_header(response, "X-Custom-Header", "MyValue");
    }
    
    char resp_body[256];
    snprintf(resp_body, 256,
        "{\n"
        "    \"id_book\": %d,\n"
        "    \"title\": \"%s\",\n"
        "    \"author\": \"%s\",\n"
        "    \"price\": %.2f\n"
        "}",
        new_book.id, new_book.title, new_book.author, new_book.price);

    
    set_response_status(response, HTTP_OK);
    set_response_json(response, resp_body);
    add_response_header(response, "X-Custom-Header", "MyValue");

}

http_response_t* process_rest_request(http_request_t *request, redisContext *c){

    http_response_t *response = create_http_response();

    // Routing basato sul metodo HTTP
    switch (request->method) {
        case HTTP_POST:
            crud_create(request, response,c);
            break;
            
        case HTTP_GET:
            crud_read(request, response,c);
            break;
            
        case HTTP_PUT:
            crud_update(request, response,c);
        case HTTP_PATCH:
            
            break;
            
        case HTTP_DELETE:
            crud_delete(request,response,c);
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