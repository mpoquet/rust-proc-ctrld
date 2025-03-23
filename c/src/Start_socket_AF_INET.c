#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>

#define PORT 8090

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};
    char *hello = "Hello from server";

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    struct stat statbuf;

    // Get the inode number
    if (fstat(server_fd, &statbuf) == 0) {
        printf("Inode number: %ld\n", (long)statbuf.st_ino);
    } else {
        perror("fstat");
    }

    printf("pid : %d\n", getpid());

    printf("Real user ID: %d\n", getuid());

    // Now start listening for the clients, here process will
    // go in sleep mode and will wait for the incoming connection
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    // Subtract 1 for the null terminator at the end
    int valread = read(new_socket, buffer, 1024 - 1);
    printf("%s\n", buffer);

    send(new_socket, hello, strlen(hello), 0);
    printf("Hello message sent\n");

    // Closing the connected socket
    close(new_socket);

    // Closing the listening socket
    close(server_fd);
    return 0;
}