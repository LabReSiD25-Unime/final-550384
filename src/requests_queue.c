#include "requests_queue.h"

request_queue_t* createQueue(int maxSize) {
    request_queue_t* q = (request_queue_t*)malloc(sizeof(request_queue_t));
    if (q == NULL) {
        printf("Errore: impossibile allocare memoria per la coda\n");
        return NULL;
    }
    q->front = NULL;
    q->rear = NULL;
    q->size = 0;

    q->maxSize = maxSize;
    q->totalProduced = 0;
    q->totalConsumed = 0;
    q->shutdownFlag = false;

    // Inizializza mutex e condition variables
    if (pthread_mutex_init(&q->mutex, NULL) != 0) {
        printf("Errore: impossibile inizializzare mutex\n");
        free(q);
        return NULL;
    }
    
    if (pthread_cond_init(&q->notEmpty, NULL) != 0) {
        printf("Errore: impossibile inizializzare condition variable notEmpty\n");
        pthread_mutex_destroy(&q->mutex);
        free(q);
        return NULL;
    }
    
    if (pthread_cond_init(&q->notFull, NULL) != 0) {
        printf("Errore: impossibile inizializzare condition variable notFull\n");
        pthread_cond_destroy(&q->notEmpty);
        pthread_mutex_destroy(&q->mutex);
        free(q);
        return NULL;
    }
    
    // Inizializza semafori
    if (sem_init(&q->emptySlots, 0, maxSize) != 0) {
        printf("Errore: impossibile inizializzare semaforo emptySlots\n");
        pthread_cond_destroy(&q->notFull);
        pthread_cond_destroy(&q->notEmpty);
        pthread_mutex_destroy(&q->mutex);
        free(q);
        return NULL;
    }
    
    if (sem_init(&q->fullSlots, 0, 0) != 0) {
        printf("Errore: impossibile inizializzare semaforo fullSlots\n");
        sem_destroy(&q->emptySlots);
        pthread_cond_destroy(&q->notFull);
        pthread_cond_destroy(&q->notEmpty);
        pthread_mutex_destroy(&q->mutex);
        free(q);
        return NULL;
    }

    return q;
}


// Funzione per verificare se la coda è vuota
bool isEmpty(request_queue_t* q) {
    return (q == NULL || q->front == NULL);
}


// Funzione per verificare se la coda è vuota (chiamata con mutex già acquisito)
bool isEmptyUnsafe(request_queue_t* q) {
    return (q->size == 0);
}

// Funzione per verificare se la coda è piena (chiamata con mutex già acquisito)
bool isFullUnsafe(request_queue_t* q) {
    return (q->size >= q->maxSize);
}

// Funzione per ottenere la dimensione della coda
int getSize(request_queue_t* q) {
    return (q == NULL) ? 0 : q->size;
}

// Funzione per aggiungere un elemento alla coda (enqueue/push)
bool enqueue(request_queue_t* q, http_request_t *request) {
    if (q == NULL) {
        printf("Errore: coda non inizializzata\n");
        return false;
    }

    // Attendi uno slot vuoto (semaforo)
    sem_wait(&q->emptySlots);
    
    // Acquisisci il mutex per accesso esclusivo
    pthread_mutex_lock(&q->mutex);

    
    // Verifica se la coda è in shutdown
    if (q->shutdownFlag) {
        pthread_mutex_unlock(&q->mutex);
        sem_post(&q->emptySlots); // Rilascia il semaforo
        return false;
    }
    
    // Attendi finché la coda non è piena (usando condition variable)
    while (isFullUnsafe(q) && !q->shutdownFlag) {
        pthread_cond_wait(&q->notFull, &q->mutex);
    }
    
    if (q->shutdownFlag) {
        pthread_mutex_unlock(&q->mutex);
        sem_post(&q->emptySlots);
        return false;
    }


    // Crea un nuovo nodo
    client_request_node_t* newNode = (client_request_node_t*)malloc(sizeof(client_request_node_t));
    if (newNode == NULL) {
        printf("Errore: impossibile allocare memoria per il nuovo nodo\n");
        return false;
    }
    
    newNode->request = *request;
    newNode->next = NULL;
    
    // Inserisci nella coda
    if (isEmptyUnsafe(q)) {
        q->front = newNode;
        q->rear = newNode;
    } else {
        q->rear->next = newNode;
        q->rear = newNode;
    }
    
    q->size++;
    q->totalProduced++;
        
    // Segnala che la coda non è vuota
    pthread_cond_signal(&q->notEmpty);
    
    // Rilascia il mutex
    pthread_mutex_unlock(&q->mutex);
    
    // Segnala che c'è un elemento pieno
    sem_post(&q->fullSlots);
    
    return true;
}

bool enqueue_node(request_queue_t* q, client_request_node_t *node) {
    if (q == NULL) {
        printf("Errore: coda non inizializzata\n");
        return false;
    }

    // Attendi uno slot vuoto (semaforo)
    sem_wait(&q->emptySlots);
    
    // Acquisisci il mutex per accesso esclusivo
    pthread_mutex_lock(&q->mutex);

    
    // Verifica se la coda è in shutdown
    if (q->shutdownFlag) {
        pthread_mutex_unlock(&q->mutex);
        sem_post(&q->emptySlots); // Rilascia il semaforo
        return false;
    }
    
    // Attendi finché la coda non è piena (usando condition variable)
    while (isFullUnsafe(q) && !q->shutdownFlag) {
        pthread_cond_wait(&q->notFull, &q->mutex);
    }
    
    if (q->shutdownFlag) {
        pthread_mutex_unlock(&q->mutex);
        sem_post(&q->emptySlots);
        return false;
    }



    
    // Inserisci nella coda
    if (isEmptyUnsafe(q)) {
        q->front = node;
        q->rear = node;
    } else {
        q->rear->next = node;
        q->rear = node;
    }
    
    q->size++;
    q->totalProduced++;
        
    // Segnala che la coda non è vuota
    pthread_cond_signal(&q->notEmpty);
    
    // Rilascia il mutex
    pthread_mutex_unlock(&q->mutex);
    
    // Segnala che c'è un elemento pieno
    sem_post(&q->fullSlots);
    
    return true;
}


// Funzione per rimuovere un elemento dalla coda (dequeue/pop)
bool dequeue(request_queue_t* q, http_request_t* request) {

    if (q == NULL || request == NULL) return false;

    // Attendi un elemento pieno (semaforo)
    sem_wait(&q->fullSlots);
    
    // Acquisisci il mutex per accesso esclusivo
    pthread_mutex_lock(&q->mutex);
    
    // Attendi finché la coda non è vuota (usando condition variable)
    while (isEmptyUnsafe(q) && !q->shutdownFlag) {
        pthread_cond_wait(&q->notEmpty, &q->mutex);
    }
    
    // Se la coda è in shutdown e vuota, termina
    if (q->shutdownFlag && isEmptyUnsafe(q)) {
        pthread_mutex_unlock(&q->mutex);
        sem_post(&q->fullSlots); // Rilascia il semaforo
        return false;
    }


    client_request_node_t* temp = q->front;
    *request = temp->request;
    
    q->front = q->front->next;
    
    // Se la coda diventa vuota
    if (q->front == NULL) {
        q->rear = NULL;
    }
    
    q->size--;
    q->totalConsumed++;
    

    free(temp);
    
    // Segnala che la coda non è piena
    pthread_cond_signal(&q->notFull);
    
    // Rilascia il mutex
    pthread_mutex_unlock(&q->mutex);
    
    // Segnala che c'è uno slot vuoto
    sem_post(&q->emptySlots);
    
    return true;
}


// Funzione per rimuovere un elemento dalla coda ma funziona piu a basso livello 
bool dequeue_node(request_queue_t* q, client_request_node_t* node) {

    if (q == NULL || node == NULL) return false;

    // Attendi un elemento pieno (semaforo)
    sem_wait(&q->fullSlots);
    
    // Acquisisci il mutex per accesso esclusivo
    pthread_mutex_lock(&q->mutex);
    
    // Attendi finché la coda non è vuota (usando condition variable)
    while (isEmptyUnsafe(q) && !q->shutdownFlag) {
        pthread_cond_wait(&q->notEmpty, &q->mutex);
    }
    
    // Se la coda è in shutdown e vuota, termina
    if (q->shutdownFlag && isEmptyUnsafe(q)) {
        pthread_mutex_unlock(&q->mutex);
        sem_post(&q->fullSlots); // Rilascia il semaforo
        return false;
    }


    client_request_node_t* temp = q->front;
    //*request = temp->request;
    *node =  *temp;
    
    
    q->front = q->front->next;
    
    // Se la coda diventa vuota
    if (q->front == NULL) {
        q->rear = NULL;
    }
    
    q->size--;
    q->totalConsumed++;
    

    free(temp);
    
    // Segnala che la coda non è piena
    pthread_cond_signal(&q->notFull);
    
    // Rilascia il mutex
    pthread_mutex_unlock(&q->mutex);
    
    // Segnala che c'è uno slot vuoto
    sem_post(&q->emptySlots);
    
    return true;
}


// Funzione per vedere il primo elemento senza rimuoverlo (peek/front)
bool peek(request_queue_t* q, http_request_t* request) {
    if (isEmpty(q)) {
        printf("Errore: coda vuota, impossibile fare peek\n");
        return false;
    }
    
    *request = q->front->request;
    return true;
}

// Funzione per vedere l'ultimo elemento senza rimuoverlo (rear)
bool peekRear(request_queue_t* q, http_request_t* request) {
    if (isEmpty(q)) {
        printf("Errore: coda vuota, impossibile fare peek rear\n");
        return false;
    }
    
    *request = q->rear->request;
    return true;
}

// Funzione per stampare tutti gli elementi della coda
void printQueue(request_queue_t* q) {
    if (isEmpty(q)) {
        printf("Coda vuota\n");
        return;
    }
    
    printf("Coda: ");
    client_request_node_t* current = q->front;
    while (current != NULL) {
        print_http_request(&current->request);
        current = current->next;
    }
    printf("(dimensione: %d)\n", q->size);
}

// Funzione per svuotare completamente la coda
void clearQueue(request_queue_t* q) {
    if (q == NULL) return;
    
    while (!isEmpty(q)) {
        http_request_t temp;
        dequeue(q, &temp);
    }
}

// Funzione per distruggere la coda e liberare tutta la memoria
void destroyQueue(request_queue_t* q) {
    if (q == NULL) return;
    
    clearQueue(q);
    free(q);
}


// Funzione thread-safe per ottenere le statistiche
void getStatistics(request_queue_t* q, int* size, int* produced, int* consumed) {
    pthread_mutex_lock(&q->mutex);
    *size = q->size;
    *produced = q->totalProduced;
    *consumed = q->totalConsumed;
    pthread_mutex_unlock(&q->mutex);
}

