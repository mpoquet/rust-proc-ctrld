#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/inotify.h>
#include <fcntl.h>
#include <sched.h>
#include "../include/clone.h"
#include <signal.h>
#include <sys/stat.h>


#define MAX_EVENTS 1024  /* Maximum number of events to process*/
#define LEN_NAME 16  /* Assuming that the length of the filename won't exceed 16 bytes*/
#define EVENT_SIZE  ( sizeof (struct inotify_event) ) /*size of one event*/
#define BUF_LEN     ( MAX_EVENTS * ( EVENT_SIZE + LEN_NAME )) /*buffer to store the data of events*/

static int inotifyFd;

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

void sigio_handler(int sig){
    char buf[BUF_LEN];

    ssize_t numRead = read(inotifyFd, buf, BUF_LEN);
    if (numRead == 0) {
        fprintf(stderr, "read() from inotify fd returned 0!");
    }

    if (numRead == -1) {
        fprintf(stderr, "read");
    }

    printf("Read %ld bytes from inotify fd\n", (long) numRead);

    /* Process all of the events in buffer returned by read() */

    for (char *p = buf; p < buf + numRead; ) {
        struct inotify_event *event = (struct inotify_event *) p;
        displayInotifyEvent(event);
        p += sizeof(struct inotify_event) + event->len;
    }
}

/*
int main (int argc, char** argv){
    inotifyFd = inotify_init();
    if (inotifyFd == -1){
        printf("error while initializing inotify");
        perror("inotify");
    }

    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sa.sa_handler = sigio_handler;
    if (sigaction(SIGIO, &sa, NULL) == -1) {
        fprintf(stderr, "sigaction");
    }

    // Set owner process that is to receive "I/O possible" signal

    if (fcntl(inotifyFd, F_SETOWN, getpid()) == -1) {
        fprintf(stderr, "fcntl(F_SETOWN)");
    }

    // Enable "I/O possible" signaling and make I/O nonblocking for file descriptor 

    int flags = fcntl(inotifyFd, F_GETFL);
    if (fcntl(inotifyFd, F_SETFL, flags | O_ASYNC | O_NONBLOCK) == -1) {
        fprintf(stderr, "fcntl(F_SETFL)");
    }

    inotify_add_watch(inotifyFd, "./", IN_CREATE | IN_ACCESS | IN_MODIFY );

    char* filepath = "Start_AF_UNIX";
    parameter_clone* param = malloc(sizeof(parameter_clone));
    if (!param) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    param->filepath=filepath;
    param->args = (char* const[]) { "Start_AF_UNIX", NULL };
    info_child* info = launch_process(1024*1024,param,SIGCHLD);

    printf("Processus parent : PID = %d, Fils = %d\n", getpid(), info->child_id);
    waitpid(info->child_id, NULL, 0);
    free(info->stack_p);

    return 1;
}*/