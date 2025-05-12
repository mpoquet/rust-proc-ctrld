#ifndef __NETWORK_H__
#define __NETWORK_H__

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include "data_structs.h" 

struct socket_info{
    int port;
    struct sockaddr_in address;
    int sockfd;
};

struct socket_info* establish_connection(int port); //exemple of function i want for the daemon 

int accept_new_connexion(struct socket_info* info);

struct buffer_info* read_socket_message(int socket);

int send_message(int socket, struct buffer_info* info);
#endif 