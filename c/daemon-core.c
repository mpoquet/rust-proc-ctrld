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

    int nochdir = 0;

    int noclose = 0;

    // glibc call to daemonize this process without a double fork
    if(daemon(nochdir, noclose))
        perror("daemon");

    // To verify, execute :
    // pgrep daemon-core
    // ps -p pid -o "user pid ppid pgid sid tty command"
    // lsof -p pid

    process_info** process_manager = create_process_manager(MAX_PROCESS);

    int communication_socket;

    struct socket_info* server_socket = establish_connection(destPort);

    int err_file = initialize_error_file("daemon_trace.txt");

    //Error file for debugging purpose
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
    edata->fd = server_socket->sockfd;
    edata->type = SOCK_CONNEXION;
    edata->sock_info=server_socket;
    ev.events=EPOLLIN;
    ev.data.ptr=edata;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, server_socket->sockfd, &ev)==-1){
        printf("error while trying to add file descriptor to epoll interest list");
        free(edata);
        return -1;
    }

    //create inotifyFD to watch files requested by user
    int inotifyFd = inotify_init();
    if (inotifyFd == -1){
        printf("error while initializing inotify");
        perror("inotify");
    }

    add_event_inotifyFd(inotifyFd, epollfd,0);

    //Add the signal SIGCHLD to the watch list for future child creation
    sigset_t mask;
    int sfd;
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);

    if (sigprocmask(SIG_SETMASK, &mask, NULL) == -1) {
        perror("sigprocmask");
        return 1;
    }

    sfd=signalfd(-1,&mask,0);
    if(sfd == -1){
        perror("signalfd");
        exit(3);
    }

    add_event_signalFd(sfd, epollfd);

    while(1){
        printf("Waiting for incoming notifications\n");
        nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }
        printf("received notification, processing it...\n");
        fflush(stdout);
        for (int i = 0; i<nfds; i++){
            if (events[i].events != 0) {
                event_data_t* edata = (event_data_t*)events[i].data.ptr;
                if (events[i].events & EPOLLIN) {
                    switch (edata->type)
                    {
                    case INOTIFYFD: {
                        printf("Inotify event received\n");
                        fflush(stdout);
                        event_data_Inotify_size* I_edata = (event_data_Inotify_size*)events[i].data.ptr;
                        handle_inotify_event(edata->fd, I_edata->size,communication_socket);
                        break;
                    }
                    
                    case SIGNALFD: {
                        printf("signal received\n");
                        printf("nb_process : %d\n", nb_process);
                        fflush(stdout);
                        struct buffer_info* res;
                        if ((res=handle_signalfd_event(edata->fd, process_manager, &nb_process)) == NULL){
                            printf("Unable to read signalfd\n");
                        }else{
                            printf("sending child terminated\n");
                            fflush(stdout);
                            send_message(communication_socket,res);
                        }
                        break;
                    }
                    
                    case SOCK_MESSAGE: {
                        printf("Incoming job...\n");
                        fflush(stdout);
                        struct buffer_info* info = read_socket_message(edata->fd);
                        uint8_t* buffer = info->buffer;
                        int size = info->size;
                        if(size==0){
                            printf("The socket has been closed\n");
                            fflush(stdout);
                            epoll_ctl(epollfd, EPOLL_CTL_DEL, edata->fd, NULL);
                            close(edata->fd);          
                            free(edata);
                            break;
                        }
                        printf("Command received\n");
                        switch (receive_message_from_user_c(buffer, size)){
                            case RUN_COMMAND:
                                //First check if there is a program to execute
                                printf("RUNCOMMAND\n");
                                com= receive_command_c(buffer,size);
                                process_command_request(process_manager,&nb_process,edata->fd,com,epollfd);
                                process_surveillance_requests(com,inotifyFd,epollfd, edata->fd);
                                free(com);
                                break;

                            case KILL_PROCESS:
                                printf("KILLPROCESS\n");
                                int pid = receive_kill_c(buffer,size);
                                printf("pid to kill : %d\n", pid);
                                fflush(stdout);
                                if(pid>0){
                                    printf("Killing process : %d\n", pid);
                                    fflush(stdout);
                                    kill(pid,SIGKILL);
                                }
                                else if (pid==0){ //Kill Daemon
                                    printf("Killing daemon\n");
                                    free_manager(process_manager, nb_process);
                                    kill(-getpgrp(), SIGTERM); // Kill all process of the group
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
                        communication_socket=new_connexion; //Temporary, assume there is only one client
                        struct buffer_info* info = send_tcpsocketlistening_to_user_c(destPort);
                        send_message(new_connexion, info);
                        break;
                    }
                    case EXECVE: {
                        printf("receiving execve notification\n");
                        fflush(stdout);
                        event_data_pipe_execve* edata = (event_data_pipe_execve*)events[i].data.ptr;
                        int res;
                        ssize_t n = read(edata->fd, &res, sizeof(res));
                        struct execve_info info_e;
                        info_e.command_name=edata->path;
                        info_e.pid=edata->pid;
                        if (n > 0) {
                            info_e.success=false;
                        } else if (n == 0) {
                            info_e.success=true;
                        } else {
                            // erreur de lecture
                        }
                        printf("sending execve termitated\n");
                        fflush(stdout);
                        struct buffer_info* info = send_execveterminated_to_user_c(&info_e);
                        send_message(communication_socket, info);
                        epoll_ctl(epollfd, EPOLL_CTL_DEL, edata->fd, NULL);
                        close(edata->fd);          
                        free(edata);
                        break;
                    }
                    default:
                        break;
                    }
                } else {                /* POLLERR | POLLHUP */
                    event_data_t* edata_test= (event_data_t*)events[i].data.ptr;
                    if(edata_test->type==EXECVE){
                        printf("receiving execve notification\n");
                        fflush(stdout);
                        event_data_pipe_execve* edata = (event_data_pipe_execve*)events[i].data.ptr;
                        int res;
                        ssize_t n = read(edata->fd, &res, sizeof(res));
                        struct execve_info info_e;
                        info_e.command_name=edata->path;
                        info_e.pid=edata->pid;
                        if (n > 0) {
                            info_e.success=false;
                        } else if (n == 0) {
                            info_e.success=true;
                        } else {
                            // reading error
                        }
                        printf("sending execve termitated\n");
                        fflush(stdout);
                        struct buffer_info* info = send_execveterminated_to_user_c(&info_e);
                        send_message(communication_socket, info);
                        epoll_ctl(epollfd, EPOLL_CTL_DEL, edata->fd, NULL);
                        close(edata->fd);          
                        free(edata);
                    }else{
                        printf("    closing fd %d\n", edata->fd);
                        if (close(events[i].data.fd) == -1)
                            perror("close");
                    }

                }
            }
        }
        
    }
    return 0;
}


   