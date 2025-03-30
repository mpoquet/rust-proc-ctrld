#define _GNU_SOURCE

#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/inotify.h>
#include "../include/clone.h"
#include <poll.h>
#include <signal.h>
#include <sys/signalfd.h>
#include <sys/wait.h>
#include <sys/stat.h>


#define INOTIFYFD 1
#define SIGNALFD 2
#define MAX_EVENTS 128

typedef struct {
    int fd;
    uint32_t type; // INOTIFYFD ou SIGNALFD
} event_data_t;


info_child child_infos[512];
int running_childs = 0;
int num_open_fds = 0;
int nfds;
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

void handle_signalfd_event(int fd){
    ssize_t s;
    struct signalfd_siginfo fdsi;
    for (;;) {
        s = read(fd, &fdsi, sizeof(fdsi));
        if (s != sizeof(fdsi)){
            perror("read");
            exit(2);
        } else if (fdsi.ssi_signo == SIGCHLD) {
            printf("Got SIGCHLD\n");
            printf("Processus parent : PID = %d, Fils = %d\n", getpid(), fdsi.ssi_pid);
            waitpid(fdsi.ssi_pid, NULL, 0);  // Attente de la fin du fils
            for (int i=0; i<running_childs;i++){
                if(child_infos[i].child_id==fdsi.ssi_pid){
                    free(child_infos[i].stack_p);
                    child_infos[i].child_id=-1;
                }
            }
            break;
        } else {
            printf("Read unexpected signal\n");
        }
    }
}

int add_inotifyFd_to_epoll(int fd, int epollfd){
    struct epoll_event* ev = malloc(sizeof(struct epoll_event));
    event_data_t *edata = malloc(sizeof(event_data_t));
    edata->fd = fd;
    edata->type = INOTIFYFD;
    ev->events=EPOLLIN;
    ev->data.ptr=edata;
    printf("adding file descriptor : %d\n", fd);
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, ev)==-1){
        printf("error while trying to add file descriptor to epoll interest list");
        return -1;
    }
    num_open_fds++;
}

int add_signalFd_to_epoll(int fd, int epollfd){
    struct epoll_event* ev = malloc(sizeof(struct epoll_event));
    event_data_t *edata = malloc(sizeof(event_data_t));
    edata->fd = fd;
    edata->type = SIGNALFD;
    ev->events=EPOLLIN;
    ev->data.ptr=edata;
    printf("adding file descriptor : %d\n", fd);
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, ev)==-1){
        printf("error while trying to add file descriptor to epoll interest list");
        return -1;
    }
    num_open_fds++;
}

int main (int argc, char** argv){
    

    int inotifyFd = inotify_init();
    if (inotifyFd == -1){
        printf("error while initializing inotify");
        perror("inotify");
    }

    printf("inotify fd : %d\n", inotifyFd);

    int epollfd = epoll_create1(0);
    
    inotify_add_watch(inotifyFd, "./", IN_CREATE | IN_ACCESS | IN_MODIFY );

    add_inotifyFd_to_epoll(inotifyFd, epollfd);

    sigset_t mask;
    int sfd;
    struct signalfd_siginfo fdsi;
    ssize_t s;

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

    printf("signalfd : %d\n", sfd);

    add_signalFd_to_epoll(sfd,epollfd);

    char* filepath = "./foo";
    parameter_clone* param = malloc(sizeof(parameter_clone));
    if (!param) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    param->filepath=filepath;
    param->args = (char* const[]) { "./foo","12", NULL };
    info_child* info = launch_process(1024*1024,param,0);
    running_childs++;
    child_infos[running_childs]=*info;

    while(num_open_fds>0){
        nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i<nfds; i++){
            if (events[i].events != 0) {
                event_data_t* edata = (event_data_t*)events[i].data.ptr;
                if (events[i].events & EPOLLIN) {
                    if (edata->type==INOTIFYFD){
                        handle_inotify_event(edata->fd);
                    } else if (edata->type==SIGNALFD){
                        handle_signalfd_event(edata->fd);
                    }
                    
                } else {                /* POLLERR | POLLHUP */
                    printf("    closing fd %d\n", edata->fd);
                    if (close(events[i].data.fd) == -1)
                        perror("close");
                        exit(3);
                    num_open_fds--;
                }
            }
        }
    }
    

    

}

