#define _GNU_SOURCE
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/signalfd.h>

/*Returns a signalfd catching I/O signals to the file created
We use signalf + fcntl beacause we want to know who wrote into the file
without forcing on to developper a specific scheme they need to follow when
sending errors.*/
int initialize_errors_tracer(const char* name){
    int fd = open(name, O_RDWR | O_CREAT | O_TRUNC | O_APPEND, S_IRWXU);
    if (fd ==-1){
        perror("open");
        return -1;
    }

    // Set owner process that is to receive "I/O possible" signal

    if (fcntl(fd, F_SETOWN, getpid()) == -1) {
        perror("fcntl(F_SETOWN)");
    }

    // Enable "I/O possible" signaling and make I/O nonblocking for file descriptor 

    int flags = fcntl(fd, F_GETFL);
    if (fcntl(fd, F_SETFL, flags | O_ASYNC | O_NONBLOCK) == -1) {
        perror("fcntl(F_SETFL)");
    }

    sigset_t mask;
    int sfd;
    ssize_t s;

    sigemptyset(&mask);
    sigaddset(&mask, SIGIO);

    if (sigprocmask(SIG_BLOCK,&mask,NULL)==-1){
        perror("sigprocmask");
        exit(1);
    }

    sfd=signalfd(-1,&mask,0);
    if(sfd == -1){
        perror("signalfd");
        exit(3);
    }


    if(dup2(fd, STDOUT_FILENO)==-1){
        perror("dup2");
        close(fd);
        return -2;
    }
    close(fd);
    return sfd;
}

