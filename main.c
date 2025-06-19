// main.c
#include "server_utils.h"
#include "http_utils.h"
#include "requests_queue.h"
#include "workers.h"


worker_pool_t *worker_pool;
redis_pool_t *redis_pool;

int main() {
    const int SERVER_PORT = 8080;
    const int MAX_EVENTS = 10;

    redis_pool =  malloc(sizeof(worker_pool_t));

    init_redis_pool(10,redis_pool);

    // Inizializza il server
    int server_fd = initialize_server(SERVER_PORT);
    if (server_fd < 0) {
        return EXIT_FAILURE;
    }
    
    int epoll_fd = init_epoll_istance();
    add_fd_to_epoll_istance(server_fd, epoll_fd, EPOLLIN);
    
    struct epoll_event events[MAX_EVENTS];
    
    printf("Server pronto per accettare connessioni\n");
    
    // Main event loop
    while (1) {
        int num_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        
        if (num_events == -1) {
            if (errno == EINTR) {
                // Segnale ricevuto, continua
                continue;
            }
            perror("epoll_wait failed");
            break;
        }
        
        if (num_events > 0) {
            if (process_epoll_events(server_fd, epoll_fd, events, num_events) < 0) {
                printf("Errore nel processare gli eventi\n");
                // Potresti decidere se continuare o uscire
            }
        }
    }
    
    // Cleanup
    cleanup_resources(server_fd, epoll_fd);
    
    return EXIT_SUCCESS;
}

