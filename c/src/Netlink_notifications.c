#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/if.h>
#include <sys/types.h>
#include <asm/types.h>
#include <linux/if_tun.h>
#include <arpa/inet.h>
#include <linux/sock_diag.h>
#include <linux/unix_diag.h> /* for UNIX domain sockets */
#include <linux/inet_diag.h> /* for IPv4 and IPv6 sockets */
#include <sys/un.h>
#include <linux/tcp.h>

#define BUFFER_SIZE 8192

int main() {
    int sock;
    struct sockaddr_nl addr;
    char buffer[BUFFER_SIZE];

    // Création du socket Netlink
    sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_SOCK_DIAG);
    if (sock < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    // Configuration de l'adresse locale
    memset(&addr, 0, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    addr.nl_groups = RTMGRP_IPV6_IFADDR | RTMGRP_IPV4_IFADDR | RTMGRP_LINK | RTMGRP_IPV6_MROUTE | RTMGRP_IPV4_ROUTE | RTMGRP_NOTIFY ;

    // Liaison du socket Netlink
    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(sock);
        return EXIT_FAILURE;
    }

    printf("Écoute des événements Netlink...\n");

    while (1) {
        struct iovec iov = {buffer, sizeof(buffer)};
        struct sockaddr_nl sa;
        struct msghdr msg = {&sa, sizeof(sa), &iov, 1, NULL, 0, 0};
        ssize_t len = recvmsg(sock, &msg, 0);

        printf("message received \n");

        if (len < 0) {
            perror("recvmsg");
            continue;
        }

        for (struct nlmsghdr *nlh = (struct nlmsghdr *)buffer;
             NLMSG_OK(nlh, len);
             nlh = NLMSG_NEXT(nlh, len)) {

            if (nlh->nlmsg_type == NLMSG_DONE)
                break;

            if (nlh->nlmsg_type == NLMSG_ERROR) {
                fprintf(stderr, "Erreur Netlink\n");
                continue;
            }

            if (nlh->nlmsg_type == SOCK_DIAG_BY_FAMILY) {
                struct unix_diag_msg *diag = NLMSG_DATA(nlh);
                if(diag->udiag_family==AF_INET){
                    struct inet_diag_msg *idm = NLMSG_DATA(nlh);
                    struct inet_diag_sockid ids = idm->id;
                    printf("Changement d'adresse IP détecté\n");
                    printf("inode : %d\n", idm->idiag_inode);
                    if (idm->idiag_state == 1){
                        printf("connexion established\n");
                    }
                    if (idm->idiag_state == 10){
                        printf("connexion listening\n");
                    }
                    if (idm->idiag_state == 7){
                        printf("connexion close\n");
                    }
                    printf("Source port : %d \n",ntohs(ids.idiag_sport));
                }else if(diag->udiag_family==AF_UNIX){
                    printf("SOCKET UNIX\n");
                }
            }
        }
    }

    close(sock);
    return EXIT_SUCCESS;
}

/*
For the moment only detect closing on TCP socket

Netlink ne notifie que les changement d'état significatif. Les close ou alors une fois qu'un connexion est établit mais pas le passage en listening ni la création
*/