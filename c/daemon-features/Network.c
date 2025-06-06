#include "../include/Network.h"
#include "../include/data_structs.h" 

struct socket_info* establish_connection(int port){
    int server_fd;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    struct socket_info* info = malloc(sizeof(struct socket_info));
    if(info==NULL){
        perror("malloc");
        exit(1);
    }

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; //Wont work for internet connexion, just to simplify testing
    address.sin_port = htons(port);

    //Making the port reusable if the server is not closed properly
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt");
        exit(1);
    }

    if (bind(server_fd, (struct sockaddr *)&address, addrlen) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Showing socket info for debug purpose
    if (getsockname(server_fd, (struct sockaddr *)&address, &addrlen) == -1) {
        perror("getsockname");
        exit(EXIT_FAILURE);
    }

    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(address.sin_addr), ip_str, INET_ADDRSTRLEN);
    printf("Serveur en écoute sur %s:%d\n", ip_str, ntohs(address.sin_port));

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Socket listening\n");

    info->address=address;
    info->port=port;
    info->sockfd=server_fd;

    return info;
}

int accept_new_connexion(struct socket_info* info){
    printf("accepting new connexion\n");
    int new_socket;
    socklen_t addrlen = sizeof(struct sockaddr_in);

    if ((new_socket = accept(info->sockfd, (struct sockaddr *)&info->address, &addrlen)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    printf("New connexion accepted\n");
    return new_socket;
}

struct buffer_info* read_socket_message(int socket) {
    struct buffer_info* info = malloc(sizeof(struct buffer_info));
    if (!info) {
        return NULL;
    }

    // read size
    if (read(socket, &info->size, sizeof(int)) != sizeof(int)) {
        free(info);
        return NULL;
    }

    // allocate data
    info->buffer = malloc(info->size);
    if (!info->buffer) {
        free(info);
        return NULL;
    }

    // read data
    if (read(socket, info->buffer, info->size) != info->size) {
        free(info->buffer);
        free(info);
        return NULL;
    }

    return info;
}

int send_message(int socket, struct buffer_info* info) {
    // allocate data
    uint8_t* temp_buffer = malloc(sizeof(int) + info->size);
    if (!temp_buffer) {
        return -1;
    }

    // append size to data
    memcpy(temp_buffer, &info->size, sizeof(int));
    memcpy(temp_buffer + sizeof(int), info->buffer, info->size);

    // send data+size
    ssize_t sent = send(socket, temp_buffer, sizeof(int) + info->size, 0);
    free(temp_buffer);

    if (sent != sizeof(int) + info->size) {
        return -1;
    }

    printf("data sent : %ld", sent);
    free(info);
    return sent;
}