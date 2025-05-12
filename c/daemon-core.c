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
#include <errno.h>

#include "./include/clone.h"
#include "./include/Errors.h"
#include "./include/Network.h"
#include "./include/events.h"
#include "./include/process_manager.h"
#include "./include/serialize_c.h"

#define MAX_EVENTS 128

int num_inotifyFD=0;
int nfds;
struct epoll_event events[MAX_EVENTS];
command* com;
int nb_process=0;

int main(int argc, char** argv){
    if (argc<2){
        printf("The daemon need a destination port to establish the TCP connection\n");
        return 0;
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
    process_info** process_manager = create_process_manager(MAX_PROCESS);

    struct socket_info* communication_socket = establish_connection(destPort);

    //TODO : Once the establish function is written do not forget to add the socket to inotify

    int err_file = initialize_error_file("daemon_trace.txt");

    if (err_file!=-1){
        dup2(err_file,STDOUT_FILENO);
        dup2(err_file,STDERR_FILENO);
    }

    printf("test\n");
    fflush(stdout);

    int epollfd = epoll_create1(0);

    //Add our communication socket to the event list
    struct epoll_event ev;
    event_data_sock *edata = malloc(sizeof(event_data_sock));
    if(edata==NULL){
        free(edata);
        perror("malloc");
        return -1;
    }
    edata->fd = communication_socket->sockfd;
    edata->type = SOCK_CONNEXION;
    edata->sock_info=communication_socket;
    ev.events=EPOLLIN;
    ev.data.ptr=edata;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, communication_socket->sockfd, &ev)==-1){
        printf("error while trying to add file descriptor to epoll interest list");
        free(edata);
        return -1;
    }
    num_inotifyFD++;

    int inotifyFd = inotify_init();
    if (inotifyFd == -1){
        printf("error while initializing inotify");
        perror("inotify");
    }

    add_event_inotifyFd(epollfd,inotifyFd,0);

    //Add the signal SIGCHLD to the watch list for future child creation
    sigset_t mask;
    int sfd;
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);

    if (sigprocmask(SIG_BLOCK,&mask,NULL)==-1){
        perror("sigprocmask");
        exit(1);
    }

    sfd=signalfd(-1,&mask,0);
    if(sfd == -1){
        perror("signalfd");
        exit(3);
    }

    add_event_signalFd(sfd, epollfd);

    while(num_inotifyFD>0){
        printf("Waiting for incoming notifications\n");
        nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }
        printf("received notification, processing it...\n");
        for (int i = 0; i<nfds; i++){
            if (events[i].events != 0) {
                event_data_t* edata = (event_data_t*)events[i].data.ptr;
                if (events[i].events & EPOLLIN) {
                    switch (edata->type)
                    {
                    case INOTIFYFD: {
                        event_data_Inotify_size* I_edata = (event_data_Inotify_size*)events[i].data.ptr;
                        handle_inotify_event(edata->fd, I_edata->size,communication_socket->sockfd);
                        break;
                    }
                    
                    case SIGNALFD: {
                        void* res;
                        if ((res=handle_signalfd_event(edata->fd, process_manager, nb_process)) == NULL){
                            printf("Unable to read signalfd\n");
                        }else{
                            send_message(edata->fd,res);
                        }
                        break;
                    }
                    
                    case SOCK_MESSAGE: {
                        printf("Incoming job...\n");
                        struct buffer_info* info = read_socket_message(edata->fd);
                        uint8_t* buffer = info->buffer;
                        int size = info->size;
                        if(size==0){
                            printf("The socket has been closed\n");
                            epoll_ctl(epollfd, EPOLL_CTL_DEL, edata->fd, NULL);
                            close(edata->fd);          
                            free(edata);
                            num_inotifyFD--;
                            break;
                        }
                        printf("Command received\n");
                        switch (receive_message_from_user_c(buffer, size)){
                            case RUN_COMMAND:
                                //First check if there is a program to execute
                                printf("RUNCOMMAND\n");
                                com= receive_command_c(buffer,size);
                                process_command_request(process_manager,&nb_process,edata->fd,com);
                                process_surveillance_requests(com,inotifyFd,epollfd, edata->fd);
                                free(com);
                                break;

                            case KILL_PROCESS:
                                printf("KILLPROCESS\n");
                                int pid = receive_kill_c(buffer,size);
                                if (pid==0){ //Kill Daemon
                                    free_manager(process_manager, nb_process);
                                    kill(-getpgrp(), SIGTERM); // Kill all process of the group
                                }else{
                                    kill(pid,SIGKILL);
                                    manager_remove_process(pid,process_manager,size);
                                    nb_process--;
                                }
                                break;

                            default :
                                printf("unexpected type of message received\n");
                                break;
                    
                        }
                        break;
                    }
                    case SOCK_CONNEXION: {
                        event_data_sock* edata_sock = (event_data_sock*)events[i].data.ptr;
                        int new_connexion = accept_new_connexion(edata_sock->sock_info);
                        //Add our communication socket to the event list
                        struct epoll_event* ev = malloc(sizeof(struct epoll_event));
                        event_data_t *edata = malloc(sizeof(event_data_t));
                        edata->fd = new_connexion;
                        edata->type = SOCK_MESSAGE;
                        ev->events=EPOLLIN;
                        ev->data.ptr=edata;
                        if (epoll_ctl(epollfd, EPOLL_CTL_ADD, new_connexion, ev)==-1){
                            printf("error while trying to add file descriptor to epoll interest list");
                            free(ev);
                            free(edata);
                            return -1;
                        }
                        num_inotifyFD++;

                        struct buffer_info* info = send_tcpsocketlistening_to_user_c(destPort);
                        send_message(new_connexion, info);
                        break;
                    }
                    default:
                        break;
                    }
                } else {                /* POLLERR | POLLHUP */
                    printf("    closing fd %d\n", edata->fd);
                    if (close(events[i].data.fd) == -1)
                        perror("close");
                    num_inotifyFD--;
                }
            }
        }
        
    }
    return 0;
}


   