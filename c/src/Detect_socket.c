#include <sys/socket.h>
#include <linux/netlink.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <asm/types.h>
#include <stdio.h>
#include <linux/rtnetlink.h>
#include <linux/if_tun.h>
#include <linux/if.h>
#include <arpa/inet.h>
#include <linux/sock_diag.h>
#include <linux/unix_diag.h> /* for UNIX domain sockets */
#include <linux/inet_diag.h> /* for IPv4 and IPv6 sockets */
#include <sys/un.h>


static int print_diag(const struct unix_diag_msg *diag, unsigned int len){

    if (len < NLMSG_LENGTH(sizeof(*diag))) {
        fputs("short response\n", stderr);
        return -1;
    }

    if (diag->udiag_family != AF_UNIX) {
        fprintf(stderr, "unexpected family %u\n", diag->udiag_family);
        return -1;
    }

    unsigned int rta_len = len - NLMSG_LENGTH(sizeof(*diag));
    unsigned int peer = 0;
    size_t path_len = 0;
    char path[sizeof(((struct sockaddr_un *) 0)->sun_path) + 1];

    for (struct rtattr *attr = (struct rtattr *) (diag + 1);
                RTA_OK(attr, rta_len); attr = RTA_NEXT(attr, rta_len)) {
        switch (attr->rta_type) {
        case UNIX_DIAG_NAME:
            if (!path_len) {
                path_len = RTA_PAYLOAD(attr);
                if (path_len > sizeof(path) - 1)
                    path_len = sizeof(path) - 1;
                memcpy(path, RTA_DATA(attr), path_len);
                path[path_len] = '\0';
            }
            break;

        case UNIX_DIAG_PEER:
            if (RTA_PAYLOAD(attr) >= sizeof(peer))
                peer = *(unsigned int *) RTA_DATA(attr);
            break;
        }
    }

    printf("inode=%u", diag->udiag_ino);

    if (peer)
        printf(", peer=%u", peer);

    if (path_len)
        printf(", name=%s%s", *path ? "" : "@",
                *path ? path : path + 1);

    putchar('\n');
    return 0;
}


// Structure to encapsulate request to netlink for unix socket
struct unix_socket_diag_request {
    struct nlmsghdr nlh;  // header Netlink
    struct unix_diag_req udr; // request for unix socket
};

int send_message(int socket){

    struct sockaddr_nl nladdr ={
        .nl_family= AF_NETLINK,
    };



    struct unix_socket_diag_request req = {
        .nlh = {
            .nlmsg_len = sizeof(req),
            .nlmsg_type = SOCK_DIAG_BY_FAMILY,
            .nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP
        },
        .udr = {
            .sdiag_family = AF_UNIX,
            .udiag_states = -1,
            .udiag_show = UDIAG_SHOW_NAME | UDIAG_SHOW_PEER | UDIAG_SHOW_VFS
        }

    };

    struct iovec iov = {
        .iov_base = &req,
        .iov_len = sizeof(req)
    };
    struct msghdr msg = {
        .msg_name = &nladdr,
        .msg_namelen = sizeof(nladdr),
        .msg_iov = &iov,
        .msg_iovlen = 1
    };

    if (sendmsg(socket, &msg, 0) <0){
        perror("sendmsg");
        return -1;
    }

    return 0;
}

int receive_message(int socket){
    struct sockaddr_nl sa;

    memset(&sa, 0, sizeof(sa));
    sa.nl_family = AF_NETLINK;
    sa.nl_groups = SOCK_DIAG_BY_FAMILY;
    bind(socket, (struct sockaddr *) &sa, sizeof(sa));

    int len;
    char buf[8192];     /* 8192 to avoid message truncation on platforms with page size > 4096 */

    struct iovec iov = { buf, sizeof(buf) };
    struct sockaddr_nl sa_recv;
    struct msghdr msg = { &sa_recv, sizeof(sa_recv), &iov, 1, NULL, 0, 0 };
    struct nlmsghdr *nh;

    while(1){
        printf("waiting for message\n");
        len = recvmsg(socket, &msg, 0);

        for (nh = (struct nlmsghdr *) buf; NLMSG_OK (nh, len);
            nh = NLMSG_NEXT (nh, len)) {
            /* The end of multipart message */
            if (nh->nlmsg_type == NLMSG_DONE){
                printf("done\n");
                fflush(stdout);
                return 0;
            }
            if (nh->nlmsg_type == NLMSG_ERROR){
                /* Do some error handling */
                printf("erreur");
            }
            else{
                struct inet_diag_msg* idm = NLMSG_DATA(nh);
                printf("uid : %d", idm->idiag_uid);
                struct inet_diag_sockid ids = idm->id;
                printf("src port : %d", ids.idiag_src);

                if (print_diag(NLMSG_DATA(nh), nh->nlmsg_len)){
                       return -1;
                }
                fflush(stdout);
                
            }
        }
    }
    
}

int main(int argc, char** argv){


    int fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_SOCK_DIAG);

    send_message(fd);

    receive_message(fd);

}