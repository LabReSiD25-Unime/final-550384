// server_utils.h

#ifndef SERVER_UTILS_H
#define SERVER_UTILS_H

#include <stdio.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/epoll.h>
#include <string.h>
#include <unistd.h>
#include"requests_queue.h"
#include "book.h"


#define MAX_CLIENTS 10000
#define BUFFER_SIZE 2048


// variabili globali
extern int server_fd;    
extern int epoll_fd;
extern int triggered_event_num;
extern int current_fd;
extern int new_client_fd;
extern size_t recived_data_size;
extern char reciving_buffer[BUFFER_SIZE];

extern struct epoll_event events[MAX_CLIENTS];
extern struct sockaddr_in client_addr;
extern struct epoll_event client_event;

extern redis_pool_t *redis_pool;

// Dichiarazioni delle funzioni
int set_nonblocking(int sockfd);
int init_server_socket(int port);
int init_epoll_istance();
int add_fd_to_epoll_istance(int fd_to_monitor, int epoll_istance, int event_type);

int handle_new_connection(int server_fd, int epoll_fd);
int handle_client_data(int client_fd);
int process_epoll_events(int server_fd, int epoll_fd, struct epoll_event *events, int num_events);
void cleanup_resources(int server_fd, int epoll_fd);
int initialize_server(int port);

#endif