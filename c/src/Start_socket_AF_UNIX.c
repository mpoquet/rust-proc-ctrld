#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <sys/stat.h>

#define MY_SOCK_PATH "./somepath"
#define LISTEN_BACKLOG 50

#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

int
main(void)
{
    int                 sfd, cfd;
    socklen_t           peer_addr_size;
    struct sockaddr_un  my_addr, peer_addr;

    sfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sfd == -1)
        handle_error("socket");

    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sun_family = AF_UNIX;
    strncpy(my_addr.sun_path, MY_SOCK_PATH,
            sizeof(my_addr.sun_path) - 1);

    if (bind(sfd, (struct sockaddr *) &my_addr,
            sizeof(my_addr)) == -1)
        handle_error("bind");

    struct stat statbuf;

    // Get the inode number
    if (fstat(sfd, &statbuf) == 0) {
        printf("Inode number: %ld\n", (long)statbuf.st_ino);
    } else {
        perror("fstat");
    }

    printf("pid : %d\n", getpid());

    printf("Real user ID: %d\n", getuid());

    if (listen(sfd, LISTEN_BACKLOG) == -1)
        handle_error("listen");

    /* Now we can accept incoming connections one
        at a time using accept(2). */

    peer_addr_size = sizeof(peer_addr);
    cfd = accept(sfd, (struct sockaddr *) &peer_addr,
                &peer_addr_size);
    if (cfd == -1)
        handle_error("accept");

    /* Code to deal with incoming connection(s)... */

    if (close(sfd) == -1)
        handle_error("close");

    if (unlink(MY_SOCK_PATH) == -1)
        handle_error("unlink");
}