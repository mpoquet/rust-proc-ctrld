#define _GNU_SOURCE
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <poll.h>
#include <signal.h>
#include <sys/signalfd.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include "./include/clone.h"
#include "./include/healthcheck.h"
#include "./include/Errors.h"
#include "./include/Network.h"
#include "./include/events.h"


#define MAX_PROCESS_GROUPS 128
#define MAX_EVENTS 128

int current_groups_id[MAX_PROCESS_GROUPS]={0};
int last_group_id;
int num_inotifyFD=0;
int nfds;
struct epoll_event events[MAX_EVENTS];
command instruction_mess;

int main(int argc, char** argv){
    if (argc<2){
        printf("The daemon need a destination port to establish the TCP connection");
    }
    int destPort = atoi(argv[1]);
    //Commenting to simplify testing
    /*
    // change to the "/" directory
    int nochdir = 0;

    // redirect standard input, output and error to /dev/null
    // this is equivalent to "closing the file descriptors"
    int noclose = 0;

    // glibc call to daemonize this process without a double fork
    if(daemon(nochdir, noclose))
        perror("daemon");

    // sleep to give us time to verify the demon is working. To verify, execute :
    // pgrep daemon-core
    // ps -p pid -o "user pid ppid pgid sid tty command"
    // lsof -p pid

    sleep(60);
    */

    int communication_socket = establish_connection(destPort);

    initialize_health_status(0);

    initialize_error_data(0,"Demon_errors_trace.txt");

    int epollfd = epoll_create1(0);

    struct epoll_event* ev = malloc(sizeof(struct epoll_event));
    event_data_t *edata = malloc(sizeof(event_data_t));
    edata->fd = communication_socket;
    edata->type = SOCKFD;
    ev->events=EPOLLIN;
    ev->data.ptr=edata;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, communication_socket, ev)==-1){
        printf("error while trying to add file descriptor to epoll interest list");
        return -1;
    }
    num_inotifyFD++;

    while(num_inotifyFD>0){
        nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i<nfds; i++){
            if (events[i].events != 0) {
                event_data_t* edata = (event_data_t*)events[i].data.ptr;
                if (events[i].events & EPOLLIN) {
                    switch (edata->type)
                    {
                    case INOTIFYFD:
                        handle_inotify_event(edata->fd);
                        break;
                    
                    case SIGNALFD:
                        handle_signalfd_event(edata->fd);
                        break;
                    
                    case SOCKFD:
                        instruction_mess = receive_message();
                    default:
                        break;
                    }
                    
                } else {                /* POLLERR | POLLHUP */
                    printf("    closing fd %d\n", edata->fd);
                    if (close(events[i].data.fd) == -1)
                        perror("close");
                        exit(3);
                    num_inotifyFD--;
                }
            }
        }
    }


    return 0;

}