// server_utils.c

#include "server_utils.h"
#include "http_utils.h"

// DEFINIZIONI delle variabili globali (solo qui!)
int server_fd;    
int epoll_fd;
int triggered_event_num;
int current_fd;
int new_client_fd;
size_t recived_data_size;
char reciving_buffer[BUFFER_SIZE];

struct epoll_event events[MAX_CLIENTS];
struct sockaddr_in client_addr;
struct epoll_event client_event;
request_queue_t *requests_q;

int set_nonblocking(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl F_GETFL failed");
        return -1;
    }
    
    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl F_SETFL failed");
        return -1;
    }
    
    return 0;
}


int init_server_socket(int port){
    
    int server_fd;
    // allows the association of a socket with a specific IP address and port.
    struct sockaddr_in serv_addr;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, MAX_CLIENTS) < 0){
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    set_nonblocking(server_fd);

    return server_fd;

}

int init_epoll_istance(){
    //creates a new epoll instance and returns a file descriptor referring to that instance.
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1)
    {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    return epoll_fd;
}


int add_fd_to_epoll_istance(int fd_to_monitor, int epoll_istance, int event_type){

    struct epoll_event event;
    event.events = event_type;
    event.data.fd = fd_to_monitor;
    
    if (epoll_ctl(epoll_istance, EPOLL_CTL_ADD, fd_to_monitor, &event) == -1)
    {
        perror("epoll_ctl: add server");
        exit(EXIT_FAILURE);
    }

    return 0;
}

// Funzioni helper per migliorare la leggibilità
 int handle_new_connection(int server_fd, int epoll_fd) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    int new_client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
    
    if (new_client_fd < 0) {
        perror("Accept failed");
        return -1;
    }
    
    printf("Client connesso con successo\n");
    
    if (set_nonblocking(new_client_fd) < 0) {
        close(new_client_fd);
        return -1;
    }
    
    if (add_fd_to_epoll_istance(new_client_fd, epoll_fd, EPOLLIN) < 0) {
        close(new_client_fd);
        return -1;
    }
    
    return 0;
}

 int handle_client_data(int client_fd) {
    char receiving_buffer[BUFFER_SIZE];
    memset(receiving_buffer, 0, BUFFER_SIZE);
    
    ssize_t received_data_size = recv(client_fd, receiving_buffer, BUFFER_SIZE - 1, 0);
    
    if (received_data_size > 0) {
        // Dati ricevuti con successo
        receiving_buffer[received_data_size] = '\0';
        
        http_request_t *request = create_http_request();
        if (!request) {
            printf("Errore nella creazione della richiesta HTTP\n");
            return -1;
        }
        
        if (parse_http_request(receiving_buffer, request) != 0) {
            printf("Errore nel parsing della richiesta HTTP\n");
            free_http_request(request);
            return -1;
        }
        
        enqueue(requests_q, request);
        printQueue(requests_q);
        
        return 0;
        
    } else if (received_data_size == 0) {
        // Client disconnesso
        printf("Client disconnesso\n");
        close(client_fd);
        return 1; // Indica disconnessione
        
    } else {
        // Errore nella recv
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("recv failed");
            close(client_fd);
            return -1;
        }
        return 0; // Non è un errore fatale
    }
}

 int process_epoll_events(int server_fd, int epoll_fd, struct epoll_event *events, int num_events) {
    for (int i = 0; i < num_events; i++) {
        int current_fd = events[i].data.fd;
        
        if (current_fd == server_fd) {
            // Nuova connessione in arrivo
            if (handle_new_connection(server_fd, epoll_fd) < 0) {
                printf("Errore nell'accettare la connessione\n");
                // Continua comunque con gli altri eventi
            }
        } else {
            // Dati pronti per la lettura da un client
            int result = handle_client_data(current_fd);
            if (result < 0) {
                printf("Errore nella gestione dei dati del client\n");
                // Continua comunque con gli altri eventi
            }
        }
    }
    
    return 0;
}

int initialize_server(int port) {
    printf("Avvio del server...\n");
    
    // Inizializza la coda delle richieste
    requests_q = createQueue(10);
    if (!requests_q) {
        printf("Errore nella creazione della coda delle richieste\n");
        return -1;
    }
    
    // Inizializza il socket del server
    int server_fd = init_server_socket(port);
    if (server_fd < 0) {
        printf("Errore nell'inizializzazione del socket del server\n");
        return -1;
    }
    
    // Inizializza epoll
    int epoll_fd = init_epoll_istance();
    if (epoll_fd < 0) {
        printf("Errore nell'inizializzazione di epoll\n");
        close(server_fd);
        return -1;
    }
    
    // Aggiungi il server socket a epoll
    if (add_fd_to_epoll_istance(server_fd, epoll_fd, EPOLLIN) < 0) {
        printf("Errore nell'aggiunta del server socket a epoll\n");
        close(server_fd);
        close(epoll_fd);
        return -1;
    }
    
    printf("Server in ascolto sulla porta %d\n", port);
    
    return server_fd; // Ritorna il file descriptor del server
}

 void cleanup_resources(int server_fd, int epoll_fd) {
    if (server_fd >= 0) {
        close(server_fd);
    }
    
    if (epoll_fd >= 0) {
        close(epoll_fd);
    }
    
    // Cleanup della coda (se hai una funzione di cleanup)
    // cleanup_queue(requests_q);
}