// main.c
#include "server_utils.h"
#include "http_utils.h"
#include "requests_queue.h"


request_queue_t *requests_q;

int main() {
    printf("Start Server...\n");

    requests_q = createQueue();

    socklen_t client_len = sizeof(client_addr);

    server_fd = init_server_socket(8080);
    epoll_fd = init_epoll_istance();
    add_fd_to_epoll_istance(server_fd, epoll_fd, EPOLLIN);

    printf("Listen to connection\n");
    while (1){

        triggered_event_num = 0;
        triggered_event_num = epoll_wait(epoll_fd, events, MAX_CLIENTS, -1);

        if(triggered_event_num == -1){
            perror("epoll_wait failed");
            exit(EXIT_FAILURE);
        }else if (triggered_event_num > 0){
            
            for(int i = 0; i < triggered_event_num; i++){

                current_fd = events[i].data.fd;

                if(current_fd == server_fd){
                    //client wants to connect to the server socket
                    new_client_fd = accept(server_fd,(struct sockaddr *)&client_addr, &client_len);

                    if(new_client_fd < 0){
                        perror("Accept failed...");
                    }else{
                        // client has been accepted successfully
                        
                        printf("client accettato\n");
                        set_nonblocking(new_client_fd);
                        add_fd_to_epoll_istance(new_client_fd, epoll_fd, EPOLLIN); // Corretto ordine parametri
                    }

                }else{
                    //client fd is ready for reading
                    memset(reciving_buffer, 0, BUFFER_SIZE);
                    recived_data_size = recv(current_fd, reciving_buffer, BUFFER_SIZE - 1, 0);

                    if(recived_data_size > 0){
                        reciving_buffer[recived_data_size] = '\0';

                        http_request_t *request = create_http_request();
                        if (!request) {
                            printf("Errore nella creazione della richiesta\n");
                            return 1;
                        }
                        
                        if (parse_http_request(reciving_buffer, request) != 0) {
                            printf("Errore nel parsing della richiesta\n");
                            free_http_request(request);
                            return 1;
                        }

                        enqueue(requests_q, request);
                        
                        printQueue(requests_q);

                    }else if(recived_data_size == 0){
                        // Client disconnesso
                        printf("Client disconnected\n");
                        close(current_fd);
                    }else{
                        // Errore nella recv
                        if(errno != EAGAIN && errno != EWOULDBLOCK){
                            perror("recv failed");
                            close(current_fd);
                        }
                    }
                }
            }
        } 
    }

    return 0; // Cambiato da 1 a 0 per successo
}