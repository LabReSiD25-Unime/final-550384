#ifndef REQUESTS_QUEUE_H
#define REQUESTS_QUEUE_H

#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include<stdbool.h>
#include <pthread.h>
#include <semaphore.h>

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
    int maxSize;

    // Sincronizzazione
    pthread_mutex_t mutex;    // Mutex per accesso esclusivo
    pthread_cond_t notEmpty;  // Condition variable per coda non vuota
    pthread_cond_t notFull;   // Condition variable per coda non piena
    sem_t emptySlots;         // Semaforo per slot vuoti
    sem_t fullSlots;
    
    int totalProduced;        // Totale elementi prodotti
    int totalConsumed;        // Totale elementi consumati
    bool shutdownFlag;        // Flag per terminazione

} request_queue_t;

request_queue_t* createQueue(int maxSize);
bool isEmpty(request_queue_t* q);
bool isEmptyUnsafe(request_queue_t* q);
bool isFullUnsafe(request_queue_t* q);
int getSize(request_queue_t* q);
bool enqueue(request_queue_t* q, http_request_t *request);
bool dequeue(request_queue_t* q, http_request_t* request);
bool peek(request_queue_t* q, http_request_t* request);
bool peekRear(request_queue_t* q, http_request_t* request);
void printQueue(request_queue_t* q);
void clearQueue(request_queue_t* q);
void destroyQueue(request_queue_t* q);
void getStatistics(request_queue_t* q, int* size, int* produced, int* consumed);

#endif