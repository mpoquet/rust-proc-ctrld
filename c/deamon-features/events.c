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

#define MAX_EVENTS 128

info_child child_infos[512];
int running_process = 0;

int findSize(char file_name[]) {
    struct stat statbuf;
    if (stat(file_name, &statbuf) == 0) {
        return statbuf.st_size;
    } else {
        printf("File Not Found!\n");
        return -1;
    }
}

//TODO gérer la remontée d'erreur
info_child* handle_clone_event(command* com, int errorfd){
    struct clone_parameters* param = extract_clone_info(com);
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

static void displayInotifyEvent(struct inotify_event *i)
{
    printf("    wd =%2d; ", i->wd);
    if (i->cookie > 0)
        printf("cookie =%4d; ", i->cookie);

    printf("mask = ");
    if (i->mask & IN_ACCESS)        printf("IN_ACCESS ");
    if (i->mask & IN_ATTRIB)        printf("IN_ATTRIB ");
    if (i->mask & IN_CLOSE_NOWRITE) printf("IN_CLOSE_NOWRITE ");
    if (i->mask & IN_CLOSE_WRITE)   printf("IN_CLOSE_WRITE ");
    if (i->mask & IN_CREATE)        printf("IN_CREATE ");
    if (i->mask & IN_DELETE)        printf("IN_DELETE ");
    if (i->mask & IN_DELETE_SELF)   printf("IN_DELETE_SELF ");
    if (i->mask & IN_IGNORED)      printf("IN_IGNORED ");
    if (i->mask & IN_ISDIR)      printf("IN_ISDIR ");
    if (i->mask & IN_MODIFY) {
        printf("IN_MODIFY ");
        printf("file size : %d",findSize(i->name));
    }        
    if (i->mask & IN_MOVE_SELF)  printf("IN_MOVE_SELF ");
    if (i->mask & IN_MOVED_FROM)    printf("IN_MOVED_FROM ");
    if (i->mask & IN_MOVED_TO)    printf("IN_MOVED_TO ");
    if (i->mask & IN_OPEN)        printf("IN_OPEN ");
    if (i->mask & IN_Q_OVERFLOW)    printf("IN_Q_OVERFLOW ");
    if (i->mask & IN_UNMOUNT)      printf("IN_UNMOUNT ");
    printf("\n");

    if (i->len > 0)
        printf("        name = %s\n", i->name);
}

void handle_inotify_event(int fd){
    char buf[4096];
    ssize_t s = read(fd, buf, sizeof(buf));
    printf("    read %zd bytes: %.*s\n",
        s, (int) s, buf);
    for(char *p = buf; p<buf+s;){
        struct inotify_event* event = (struct inotify_event *)p;
        displayInotifyEvent(event);
        p += sizeof(struct inotify_event) + event->len;
    }

}

//TODO modifer pour utiliser autre chose que running process et child info.
int handle_SIGCHLD(struct signalfd_siginfo fdsi){
    int wstatus;
    int exit_status;
    waitpid(fdsi.ssi_pid, &wstatus, 0);  // Attente de la fin du fils
    
    //TODO Gérer le renvoi d'érreur dans les deux cas. Il faut une fonction pour trouver un groupe a partir d'un pid pour pouvoir envoyer les bonnes infos.
    if(WIFEXITED(wstatus)){
        exit_status = WEXITSTATUS(wstatus);
        struct child_err* data = malloc(sizeof(struct child_err));
        if (data==NULL){
            printf("Unable to allocate memory for error message");
        }else{
            data->com=NULL;
            data->group_id=0;
            data->pid=0;
            send_error(CHILD_EXITED,(void*)data);
        }
    }else if (WIFSIGNALED(wstatus)){
        exit_status= WTERMSIG(wstatus);
        struct child_err* data = malloc(sizeof(struct child_err));
        if (data==NULL){
            printf("Unable to allocate memory for error message");
        }else{
            data->com=NULL;
            data->group_id=0;
            data->pid=0;
            send_error(CHILD_SIGNALED,(void*)data);
        }        
    }
    for (int i=0; i<running_process;i++){
        if(child_infos[i].child_id==fdsi.ssi_pid){
            free(child_infos[i].stack_p);
            child_infos[i].child_id=-1;
        }
    }
    return exit_status;
}

int handle_signalfd_event(int fd){
    ssize_t s;
    struct signalfd_siginfo fdsi;
    int exit_status;
    for (;;) {
        s = read(fd, &fdsi, sizeof(fdsi));
        if (s != sizeof(fdsi)){
            perror("read");
            return -1;
            
        } 
        switch (fdsi.ssi_signo)
        {
        case SIGCHLD:
            exit_status = handle_SIGCHLD(fdsi);
            break;
        
        default:
            printf("Read unexpected signal\n");
            break;
        }
    }
    return exit_status;
}

int add_event_inotifyFd(int fd, int epollfd){
    struct epoll_event* ev = malloc(sizeof(struct epoll_event));
    event_data_t *edata = malloc(sizeof(event_data_t));
    edata->fd = fd;
    edata->type = INOTIFYFD;
    ev->events=EPOLLIN;
    ev->data.ptr=edata;
    printf("adding file descriptor : %d\n", fd);
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, ev)==-1){
        printf("error while trying to add file descriptor to epoll interest list");
        free(edata);
        free(ev);
        return -1;
    }
    return 0;
}

int add_event_signalFd(int fd, int epollfd){
    struct epoll_event* ev = malloc(sizeof(struct epoll_event));
    event_data_t *edata = malloc(sizeof(event_data_t));
    edata->fd = fd;
    edata->type = SIGNALFD;
    ev->events=EPOLLIN;
    ev->data.ptr=edata;
    printf("adding file descriptor : %d\n", fd);
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, ev)==-1){
        printf("error while trying to add file descriptor to epoll interest list");
        free(edata);
        free(ev);
        return -1;
    }
    return 0;
}