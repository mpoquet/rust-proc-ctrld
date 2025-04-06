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

#define MAX_EVENTS 128

info_child child_infos[512];
int running_process = 0;
int num_open_fds = 0;
struct epoll_event events[MAX_EVENTS];

int findSize(char file_name[]) {
    struct stat statbuf;
    if (stat(file_name, &statbuf) == 0) {
        return statbuf.st_size;
    } else {
        printf("File Not Found!\n");
        return -1;
    }
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
    printf("reading\n");
    fflush(stdout);
    ssize_t s = read(fd, buf, sizeof(buf));
    printf("    read %zd bytes: %.*s\n",
        s, (int) s, buf);
    for(char *p = buf; p<buf+s;){
        struct inotify_event* event = (struct inotify_event *)p;
        displayInotifyEvent(event);
        p += sizeof(struct inotify_event) + event->len;
    }

}

void handle_SIGCHLD(struct signalfd_siginfo fdsi){
    //printf("Got SIGCHLD\n");
    //printf("Processus parent : PID = %d, Fils = %d\n", getpid(), fdsi.ssi_pid);
    waitpid(fdsi.ssi_pid, NULL, 0);  // Attente de la fin du fils
    for (int i=0; i<running_process;i++){
        if(child_infos[i].child_id==fdsi.ssi_pid){
            free(child_infos[i].stack_p);
            child_infos[i].child_id=-1;
        }
    }
}

void handle_signalfd_event(int fd){
    ssize_t s;
    struct signalfd_siginfo fdsi;
    for (;;) {
        s = read(fd, &fdsi, sizeof(fdsi));
        if (s != sizeof(fdsi)){
            perror("read");
            exit(2);
            
        } 
        switch (fdsi.ssi_signo)
        {
        case SIGCHLD:
            handle_SIGCHLD(fdsi);
            break;
        
        default:
            printf("Read unexpected signal\n");
            break;
        }
    }
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