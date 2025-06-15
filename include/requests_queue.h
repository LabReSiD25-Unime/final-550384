#ifndef REQUESTS_QUEUE_H
#define REQUESTS_QUEUE_H
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include<stdbool.h>
#include "http_utils.h"

typedef struct client_request_node_t{   
    int client_fd;
    http_request_t request;
    struct client_request_node_t*next;
}client_request_node_t;

typedef struct {
    client_request_node_t* front;  // Puntatore al primo elemento (per dequeue)
    client_request_node_t* rear;   // Puntatore all'ultimo elemento (per enqueue)
    int size;     // Numero di elementi nella coda
} request_queue_t;

request_queue_t* createQueue();
bool isEmpty(request_queue_t* q);
int getSize(request_queue_t* q);
bool enqueue(request_queue_t* q, http_request_t *request);
bool dequeue(request_queue_t* q, http_request_t* request);
bool peek(request_queue_t* q, http_request_t* request);
bool peekRear(request_queue_t* q, http_request_t* request);
void printQueue(request_queue_t* q);
void clearQueue(request_queue_t* q);
void destroyQueue(request_queue_t* q);



#endif