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
#include "./include/group_manager.h"


#define MAX_PROCESS_GROUPS 128
#define MAX_PROCESS_BY_GROUPS 8
#define MAX_EVENTS 128

int num_inotifyFD=0;
int nfds;
struct epoll_event events[MAX_EVENTS];
command* instruction_mess;

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
    group_info** group_manager = create_group_manager(MAX_PROCESS_GROUPS);

    int communication_socket = establish_connection(destPort);

    if(manager_add_group(0,group_manager,MAX_PROCESS_GROUPS, "Demon_errors_trace.txt")==-1){
        struct file_err* data = malloc(sizeof(struct file_err));
        if (data==NULL){
            printf("Unable to allocate memory for error message");
        }else{
            data->filepath="Demon_errors_trace.txt";
            data->group_id=0;
            data->pid=0;
            send_error(FILE_CREATION,(void*)data);
        }

    }
    dup2(group_manager[0]->err_file,STDOUT_FILENO);

    int epollfd = epoll_create1(0);

    //Add our communication socket to the event list
    struct epoll_event* ev = malloc(sizeof(struct epoll_event));
    event_data_t *edata = malloc(sizeof(event_data_t));
    edata->fd = communication_socket;
    edata->type = SOCKFD;
    ev->events=EPOLLIN;
    ev->data.ptr=edata;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, communication_socket, ev)==-1){
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
            error_code=0;
            if (events[i].events != 0) {
                event_data_t* edata = (event_data_t*)events[i].data.ptr;
                if (events[i].events & EPOLLIN) {
                    switch (edata->type)
                    {
                    case INOTIFYFD:
                        handle_inotify_event(edata->fd);
                        break;
                    
                    case SIGNALFD:
                        if (handle_signalfd_event(edata->fd) == -1){
                            printf("Unable to read signalfd\n");
                        }
                        break;
                    
                    case SOCKFD:
                        instruction_mess = receive_message();
                        if(instruction_mess==NULL){
                            printf("An error has occured while trying to read the communication socket\n");
                            break;
                        }
                        switch (instruction_mess->type){
                            case INOTIFY:
                                break;
                    
                            case WATCH_SOCKET:
                                break;
                    
                            case CLONE:
                                struct clone_parameters* param = extract_clone_info(instruction_mess);
                                char buffer[20]="errFile";
                                //On créer un fichier avec un nom unique normalement. Pours ça il faut que les valeurs soit proprement initalisé.
                                if(snprintf(buffer, sizeof(buffer), "%d", param->group_id)){
                                    printf("Error while formatting the name of the errFile for the group %d\n", param->group_id);
                                }
                                int n = manager_add_group(param->group_id,group_manager,MAX_PROCESS_GROUPS,buffer);
                                if (n==-1){
                                    struct group_full* data = malloc(sizeof(struct group_full));
                                    if (data==NULL){
                                        printf("Unable to allocate memory for error message");
                                    }else{
                                        data->com=instruction_mess;
                                        data->group_id=0;
                                        send_error(GROUP_FULL,(void*)data);
                                    }
                                }else{
                                    info_child* res = handle_clone_event(param,group_manager[n]->err_file);
                                    if(res==NULL){
                                        struct clone_err* data = malloc(sizeof(struct clone_err));
                                        if (data==NULL){
                                            printf("Unable to allocate memory for error message");
                                        }else{
                                            data->com=instruction_mess;
                                            data->group_id=0;
                                            send_error(CLONE_ERR,(void*)data);
                                        }
                                    }else{
                                        if(manager_add_process(res->child_id,group_manager[n],*instruction_mess,res->stack_p)==-1){
                                            struct group_process_full* data = malloc(sizeof(struct group_process_full));
                                            if (data==NULL){
                                                printf("Unable to allocate memory for error message");
                                            }else{
                                                data->com=instruction_mess;
                                                data->group_id=0;
                                                data->pid=res->child_id;
                                                send_error(GROUP_PROCESS_FULL,(void*)data);
                                            }
                                        }

                                    }
                                    free(res);
                                }
                                free(param);
                                break;

                            default :
                                printf("unexpected type of message received\n");
                                break;
                    
                        }

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