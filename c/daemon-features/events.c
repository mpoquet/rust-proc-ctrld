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
#include "../include/clone.h"
#include "../include/events.h"
#include <errno.h>
#include "../include/Errors.h"
#include "../include/Detect_socket.h"


#define MAX_EVENTS 128

int findSize(char file_name[]) {
    struct stat statbuf;
    if (stat(file_name, &statbuf) == 0) {
        return statbuf.st_size;
    } else {
        printf("File Not Found!\n");
        return -1;
    }
}

info_child* handle_clone_event(struct clone_parameters* param, int errorfd){
    execve_parameter* p = malloc(sizeof(execve_parameter));
    if(p==NULL){
        return NULL;
    }
    p->filepath=param->pathname;
    p->args=param->args;
    p->envp=param->envp;
    p->error_file=errorfd;
    info_child* res = launch_process(param->stack_size,p,param->flags);
    free(param);
    free(p);
    return res;
}

static void process_Event(struct inotify_event *i, struct InotifyPathUpdated* info, int size)
{
    info->path=i->name;
    info->size_limit=size;
    info->size=size;
    if (i->mask & IN_ACCESS)        info->event=ACCESSED;
    if (i->mask & IN_CREATE)        info->event=CREATED;
    if (i->mask & IN_DELETE)        info->event=DELETED;
    if (i->mask & IN_MODIFY) {
        info->event=MODIFIED;
        if((info->size=findSize(i->name))>size){
            info->event=SIZE_REACHED;
        }
    }
}

void handle_inotify_event(int fd, int size, int com_sock){
    struct InotifyPathUpdated info;
    char buf[512];
    struct buffer_info* data;
    ssize_t s = read(fd, buf, sizeof(buf));
    printf("read %zd bytes: %.*s\n",s, (int) s, buf);
    for(char *p = buf; p<buf+s;){
        struct inotify_event* event = (struct inotify_event *)p;
        process_Event(event,&info, size);
        data = send_inotifypathupdated_to_user_c(&info);
        send_message(com_sock,data);
        p += sizeof(struct inotify_event) + event->len;
    }

}

void process_command_request(process_info **process_manager, int* nb_process, int com_sock, command* com ){
    int err_file;
    printf("nbProcess : %d", *nb_process);
    struct clone_parameters* param = extract_clone_parameters(com);
    if(param!=NULL){
        char buffer[20]="errFile";
        //On créer un fichier avec un nom unique normalement. Pours ça il faut que les valeurs soit proprement initalisé.
        if(snprintf(buffer, sizeof(buffer), "%d-%s", *nb_process, param->pathname)){
            printf("Error while formatting the name of the errFile for the process %s\n", param->pathname);
        }
        err_file = initialize_error_file(buffer);
        err_file = err_file==-1?STDOUT_FILENO:err_file;
        info_child* res;
        res = handle_clone_event(param,err_file);
        if(res==NULL){
            send_message(com_sock, (void*)send_childcreationerror_to_user_c((uint32_t)errno));
        }else{
            if(manager_add_process(res->child_id, process_manager, err_file, res->stack_p, *nb_process)){
                (*nb_process)++;
            }
            printf("process successfully launched\n");
            struct buffer_info* result_message = send_processlaunched_to_user_c(res->child_id);
            send_message(com_sock,(void*)result_message);
            free(res);
        }
    }else{
        send_message(com_sock, (void*)send_childcreationerror_to_user_c((uint32_t)errno));
    }
}

//Todo add the network part when serialiation is finished
void process_surveillance_requests(command* com, int InotifyFd, int epollfd, int communication_socket){
    struct inotify_parameters* I_param;
    struct buffer_info* info;
    int* destport;
    if(com->to_watch_size>0){
        for(int i=0; i< com->to_watch_size;i++){
            switch (com->to_watch[i].event)
            {
            case INOTIFY:
                I_param = (struct inotify_parameters*) com->to_watch[i].ptr_event;
                if(I_param->size>0){
                    int inotifyFd = inotify_init();
                    if (inotifyFd == -1){
                        printf("error while initializing inotify");
                        perror("inotify");
                    }
                    //Create a new instance of inotifyFD that will store in epoll the size that trigger an event
                    inotify_add_watch(InotifyFd, I_param->root_paths, I_param->mask);
                    add_event_inotifyFd(inotifyFd,epollfd, I_param->size);
                }else{
                    //if size<0 no need to store size value and no need to create new inotify instance
                    inotify_add_watch(InotifyFd, I_param->root_paths, I_param->mask);
                }
                info = send_inotifywatchlistupdated_to_user_c(I_param->root_paths);
                send_message(communication_socket,info);
                break;
            case WATCH_SOCKET:
                destport = com->to_watch[i].ptr_event;
                info = send_socketwatched_to_user_c(*destport);
                send_message(communication_socket,info);
                detect_socket(*destport, communication_socket);
                break;
            
            default:
                printf("Unexpected surveillance type");
                break;
            }
        }
    }

}

//Return either the struct with clone parameters or NULL in case of malloc error
//Or if no program to execute has been provided
struct clone_parameters* extract_clone_parameters(command* com){
    if(com->args!=NULL){
        struct clone_parameters* param = malloc(sizeof(struct clone_parameters));
        if(param==NULL){
            perror("malloc");
            return NULL;
        }

        param->args=com->args;
        param->envp=com->envp;
        param->flags=com->flags;
        param->pathname=com->path;
        param->stack_size=com->stack_size;
        return param;
    }
    return NULL;
}

struct buffer_info* handle_SIGCHLD(struct signalfd_siginfo fdsi, process_info** manager, int size){
    int wstatus;
    int exit_status;
    waitpid(fdsi.ssi_pid, &wstatus, 0);
    
    if(WIFEXITED(wstatus)){
        exit_status = WEXITSTATUS(wstatus);
    }else if (WIFSIGNALED(wstatus)){
        exit_status= WTERMSIG(wstatus);     
    }
    printf("child : %d exited with status : %d\n", fdsi.ssi_pid, exit_status);
    manager_remove_process(fdsi.ssi_pid,manager,size);
    struct buffer_info* info = send_processterminated_to_user_c((int32_t)fdsi.ssi_pid,(uint32_t)exit_status);
    return info;
}

struct buffer_info* handle_signalfd_event(int fd, process_info** manager, int size){
    ssize_t s;
    struct signalfd_siginfo fdsi;
    struct buffer_info* res = NULL;
    s = read(fd, &fdsi, sizeof(fdsi));
    if (s != sizeof(fdsi)){
        perror("read");
        return NULL;
    } 
    switch (fdsi.ssi_signo)
    {
    case SIGCHLD:
        res = handle_SIGCHLD(fdsi, manager, size);
        break;
    
    default:
        printf("Read unexpected signal\n");
        break;
    }
    return res;

}

int add_event_inotifyFd(int fd, int epollfd, int size){
    struct epoll_event ev;
    event_data_Inotify_size *edata = malloc(sizeof(event_data_Inotify_size));
    if (edata == NULL) {
        perror("malloc");
        close(fd);
        return -1;
    }
    edata->fd = fd;
    edata->type = INOTIFYFD;
    edata->size=size;
    ev.events=EPOLLIN;
    ev.data.ptr=edata;
    printf("adding file descriptor : %d\n", fd);
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev)==-1){
        perror("epoll_ctl");
        printf("error while trying to add file descriptor to epoll interest list");
        free(edata);
    }
    return 0;
}

int add_event_signalFd(int fd, int epollfd) {
    struct epoll_event ev;
    event_data_t *edata = malloc(sizeof(event_data_t));
    if (edata == NULL) {
        perror("malloc");
        close(fd);
        return -1;
    }

    edata->fd = fd;
    edata->type = SIGNALFD;
    ev.events = EPOLLIN;
    ev.data.ptr = edata;
    
    printf("adding file descriptor : %d\n", fd);
    
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev) == -1) {
        perror("epoll_ctl");
        free(edata);
        close(fd);
        return -1;
    }
    
    return 0;
}