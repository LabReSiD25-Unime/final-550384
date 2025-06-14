// server_utils.c

#include "server_utils.h"

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

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, MAX_CLIENTS) < 0)
    {
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
