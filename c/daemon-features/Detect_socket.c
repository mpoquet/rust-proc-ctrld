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
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "../include/Detect_socket.h"
#include <pthread.h>


int timeout=0;
int found=0;

void timeout_handler(int signum){
    timeout=1;
}

static int process_inet_diag(const struct inet_diag_msg *diag, int port_filter)
{
    // On récupère les ports au format host
    uint16_t sport = ntohs(diag->id.idiag_sport);
    uint16_t dport = ntohs(diag->id.idiag_dport);

    // Si ça vous intéresse, on peut aussi récupérer adresses et uid
    char src_addr[INET6_ADDRSTRLEN], dst_addr[INET6_ADDRSTRLEN];
    if (diag->idiag_family == AF_INET) {
        inet_ntop(AF_INET, &diag->id.idiag_src, src_addr, sizeof(src_addr));
        inet_ntop(AF_INET, &diag->id.idiag_dst, dst_addr, sizeof(dst_addr));
    } else {
        inet_ntop(AF_INET6, &diag->id.idiag_src, src_addr, sizeof(src_addr));
        inet_ntop(AF_INET6, &diag->id.idiag_dst, dst_addr, sizeof(dst_addr));
    }

    if (sport == port_filter || dport == port_filter) {
        printf("[%s] %s:%u → %s:%u (inode %u, uid %u)\n",
               diag->idiag_family == AF_INET ? "IPv4" : "IPv6",
               src_addr, sport,
               dst_addr, dport,
               diag->idiag_inode,
               diag->idiag_uid);
        found=1;
        return 0;
    }
    return 0;
}

static int recv_tcp_diag(int nl_sock, int port_filter)
{
    char buf[8192];
    struct iovec iov = { buf, sizeof(buf) };
    struct sockaddr_nl sa;
    struct msghdr msg = {
        .msg_name    = &sa,
        .msg_namelen = sizeof(sa),
        .msg_iov     = &iov,
        .msg_iovlen  = 1,
    };

    int len;
    struct nlmsghdr *nh;
    while ((len = recvmsg(nl_sock, &msg, 0)) > 0) {
        for (nh = (struct nlmsghdr *)buf;
             NLMSG_OK(nh, len);
             nh = NLMSG_NEXT(nh, len))
        {
            if (nh->nlmsg_type == NLMSG_DONE)
                return 0;
            if (nh->nlmsg_type == NLMSG_ERROR) {
                fprintf(stderr, "Netlink error\n");
                return -1;
            }

            struct inet_diag_msg *diag = NLMSG_DATA(nh);
            if (process_inet_diag(diag, port_filter))
                return 1;
        }
    }
    return -1;
}



static int send_tcp_diag(int nl_sock, int family, uint16_t port_filter)
{
    struct {
        struct nlmsghdr        nlh;
        struct inet_diag_req_v2 req;
    } packet;

    memset(&packet, 0, sizeof(packet));

    // 1) Header Netlink
    packet.nlh.nlmsg_len   = sizeof(packet);
    packet.nlh.nlmsg_type  = SOCK_DIAG_BY_FAMILY;
    packet.nlh.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;

    // 2) Netlink request
    packet.req.sdiag_family   = family;       // AF_INET or AF_INET6
    packet.req.sdiag_protocol = IPPROTO_TCP;  // Only TCP
    packet.req.idiag_states   = 0x0002 | 0x0400; //bit for TCP_ESTABLISHED and TCP_LISTEN
    packet.req.idiag_ext      = 0;         
    packet.req.id.idiag_sport = htons(port_filter); // filter by port

    struct sockaddr_nl sa = {
        .nl_family = AF_NETLINK
    };
    struct iovec iov = {
        .iov_base = &packet,
        .iov_len  = sizeof(packet)
    };
    struct msghdr msg = {
        .msg_name    = &sa,
        .msg_namelen = sizeof(sa),
        .msg_iov     = &iov,
        .msg_iovlen  = 1
    };

    if (sendmsg(nl_sock, &msg, 0) < 0) {
        perror("sendmsg");
        return -1;
    }
    return 0;
}

void search_TCP_connection(int port, int communication_socket){

    signal(SIGALRM, timeout_handler);

    // Socket Netlink SOCK_DIAG
    int nl_sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_SOCK_DIAG);
    if (nl_sock < 0) {
        perror("socket");
        exit(1);
    }

    alarm(10); //if nothing is detected after 10 seconds exit

    while(!found && !timeout){
        send_tcp_diag(nl_sock, AF_INET, port); //send message for IPV4 and IPV6
        send_tcp_diag(nl_sock, AF_INET6, port);
        recv_tcp_diag(nl_sock,port);
        sleep(1);
    }

    //TODO : Envoyer un message au client en cas de detection ou pas
    if(found==1){
        printf("Socket found\n");

    }else{
        printf("Socket not found\n");

    }

}

struct detector_args {
    int port;
    int communication_socket;
};

static void* detector_thread(void *arg) {
    struct detector_args *args = arg;
    // On appelle la fonction bloquante. Quand elle retourne, le thread meurt.
    search_TCP_connection(args->port, args->communication_socket);
    free(arg);
    exit(0);
    return NULL;
}

void detect_socket(int port, int comm_sock){
    struct detector_args *args = malloc(sizeof(*args));
    if (!args) {
        perror("malloc");
        exit(1);
    }

    args->port = port;
    args->communication_socket = comm_sock;

    // Création du thread en "detached" pour qu'il se nettoie tout seul
    pthread_t tid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (pthread_create(&tid, &attr, detector_thread, args) != 0) {
        perror("pthread_create");
        free(args);
    }
    pthread_attr_destroy(&attr);
}

// int main(){

//     int server_fd;
//     struct sockaddr_in address;
//     socklen_t addrlen = sizeof(address);

//     // Creating socket file descriptor
//     if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
//         perror("socket failed");
//         exit(EXIT_FAILURE);
//     }

//     address.sin_family = AF_INET;
//     address.sin_addr.s_addr = htonl(INADDR_LOOPBACK); //Wont work for internet connexion, just to simplify testing
//     address.sin_port = htons(8080);

//     //Making the port reusable if the server is not closed properly
//     int opt = 1;
//     if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
//         perror("setsockopt");
//         exit(1);
//     }

//     if (bind(server_fd, (struct sockaddr *)&address, addrlen) < 0) {
//         perror("bind failed");
//         exit(EXIT_FAILURE);
//     }

//     // Showing socket info for debug purpose
//     if (getsockname(server_fd, (struct sockaddr *)&address, &addrlen) == -1) {
//         perror("getsockname");
//         exit(EXIT_FAILURE);
//     }

//     char ip_str[INET_ADDRSTRLEN];
//     inet_ntop(AF_INET, &(address.sin_addr), ip_str, INET_ADDRSTRLEN);
//     printf("Serveur en écoute sur %s:%d\n", ip_str, ntohs(address.sin_port));

//     if (listen(server_fd, 3) < 0) {
//         perror("listen");
//         exit(EXIT_FAILURE);
//     }

//     detect_socket(8080, 12);

//     sleep(5);

//     close(server_fd);


//     return 0;

// }