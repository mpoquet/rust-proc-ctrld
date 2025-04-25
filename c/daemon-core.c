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
#include "./include/healthcheck.h"
#include "./include/Errors.h"
#include "./include/Network.h"
#include "./include/events.h"
#include "./include/process_manager.h"


#define MAX_EVENTS 128

int num_inotifyFD=0;
int nfds;
struct epoll_event events[MAX_EVENTS];
command* com;
int nb_process=0;

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
    process_info** process_manager = create_process_manager(MAX_PROCESS);

    struct socket_info* communication_socket = establish_connection(destPort);

    //TODO : Once the establish function is written do not forget to add the socket to inotify

    int err_file = initialize_error_file("daemon_trace.txt");

    if (err_file!=-1){
        dup2(err_file,STDOUT_FILENO);
    }

    int epollfd = epoll_create1(0);

    //Add our communication socket to the event list
    struct epoll_event* ev = malloc(sizeof(struct epoll_event));
    event_data_sock *edata = malloc(sizeof(event_data_sock));
    edata->fd = communication_socket->sockfd;
    edata->type = SOCK_CONNEXION;
    edata->sock_info=communication_socket;
    ev->events=EPOLLIN;
    ev->data.ptr=edata;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, communication_socket->sockfd, ev)==-1){
        printf("error while trying to add file descriptor to epoll interest list");
        free(ev);
        free(edata);
        return -1;
    }
    num_inotifyFD++;


    //Add the signal SIGCHLD to the watch list for future child creation
    sigset_t mask;
    int sfd;
    struct signalfd_siginfo fdsi;
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

    int error_code;

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
                        void* res;
                        if ( (res=handle_signalfd_event(edata->fd, process_manager, nb_process)) == NULL){
                            printf("Unable to read signalfd\n");
                        }
                        send_message(edata->fd,res,sizeof(struct buffer_info));
                        break;
                    
                    case SOCK_MESSAGE:
                        char buffer[1024];
                        int size = read_socket(edata->fd, buffer);
                        if(size==0){
                            printf("An error has occured while trying to read the communication socket\n");
                            break;
                        }
                        switch (receive_message_from_user(buffer)){
                            case RUN_COMMAND:
                                com= receive_command(buffer,size);
                                struct clone_parameters* param = extract_clone_parameters(com);
                                char buffer[20]="errFile";
                                //On créer un fichier avec un nom unique normalement. Pours ça il faut que les valeurs soit proprement initalisé.
                                if(snprintf(buffer, sizeof(buffer), "%d-%s", nb_process, param->pathname)){
                                    printf("Error while formatting the name of the errFile for the process %s\n", param->pathname);
                                }
                                err_file = initialize_error_file(buffer);
                                err_file = err_file==-1?STDOUT_FILENO:err_file;
                                info_child* res;
                                res = handle_clone_event(param,err_file);
                                if(res==NULL){
                                    send_message(edata->fd, (void*)send_childcreationerror_to_user(errno), sizeof(struct buffer_info));
                                }else{
                                    if(manager_add_process(res->child_id, process_manager, *com, res->stack_p, nb_process)){
                                        nb_process++;
                                    }
                                    struct buffer_info* result_message = send_processlaunched_to_user(res->child_id);
                                    send_message(edata->fd,(void*)result_message,sizeof(struct buffer_info));
                                }
                                free(res);
                                free(param);
                                free(com);
                                break;

                            case KILL_PROCESS:
                                int pid = receive_kill(buffer,size);
                                if (pid==0){ //Kill Daemon
                                    free_manager(process_manager, nb_process);
                                    kill(-getpgrp(), SIGTERM); // Kill all process of the group
                                }else{
                                    kill(pid,SIGKILL);
                                    manager_remove_process(pid,process_manager,size);
                                }
                                break;

                            default :
                                printf("unexpected type of message received\n");
                                break;
                    
                        }
                    case SOCK_CONNEXION:
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

                        struct buffer_info* info = send_tcpsocketlistening_to_user(destPort);
                        send_message(new_connexion,(void*)info,sizeof(struct buffer_info));
                        break;

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