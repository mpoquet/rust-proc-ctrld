#ifndef __NETWORK_H__
#define __NETWORK_H__

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

struct socket_info{
    int port;
    struct sockaddr_in address;
    int sockfd;
};

struct socket_info* establish_connection(int port); //exemple of function i want for the daemon 

int accept_new_connexion(struct socket_info* info);

int send_message(int socket, void* buffer, int size);

int read_socket(int serveur_fd, void* buffer, int size);

#endif 