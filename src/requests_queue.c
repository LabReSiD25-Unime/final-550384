#include "requests_queue.h"

request_queue_t* createQueue() {
    request_queue_t* q = (request_queue_t*)malloc(sizeof(request_queue_t));
    if (q == NULL) {
        printf("Errore: impossibile allocare memoria per la coda\n");
        return NULL;
    }
    q->front = NULL;
    q->rear = NULL;
    q->size = 0;
    return q;
}


// Funzione per verificare se la coda è vuota
bool isEmpty(request_queue_t* q) {
    return (q == NULL || q->front == NULL);
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
    
    // Crea un nuovo nodo
    client_request_node_t* newNode = (client_request_node_t*)malloc(sizeof(client_request_node_t));
    if (newNode == NULL) {
        printf("Errore: impossibile allocare memoria per il nuovo nodo\n");
        return false;
    }
    
    newNode->request = *request;
    newNode->next = NULL;
    
    // Se la coda è vuota
    if (isEmpty(q)) {
        q->front = newNode;
        q->rear = newNode;
    } else {
        // Aggiungi il nuovo nodo alla fine
        q->rear->next = newNode;
        q->rear = newNode;
    }
    
    q->size++;
    return true;
}

// Funzione per rimuovere un elemento dalla coda (dequeue/pop)
bool dequeue(request_queue_t* q, http_request_t* request) {
    if (isEmpty(q)) {
        printf("Errore: tentativo di dequeue su coda vuota\n");
        return false;
    }
    
    client_request_node_t* temp = q->front;
    *request = temp->request;
    
    q->front = q->front->next;
    
    // Se la coda diventa vuota
    if (q->front == NULL) {
        q->rear = NULL;
    }
    
    free(temp);
    q->size--;
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

