#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include "../include/clone.h"
#include <sys/signalfd.h>
#include <signal.h>

#define STACK_SIZE 1024*1024


int child_function(void *arg) {
    printf("Processus enfant : PID = %d, PPID = %d\n", getpid(), getppid());
    parameter_clone* params = (parameter_clone *)arg;

    if (execve((const char*) params->filepath, (char* const*) params->args, NULL)==-1){
        printf("Something went wrong when opening executing program");
        perror("execve");
    };

    return 0;
}

info_child* launch_process(int stack_size, parameter_clone* parameters, int flags){
    info_child* info_c = malloc(sizeof(info_child));
    if (!info_c){
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    void *stack = malloc(stack_size);
    if (!stack) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    info_c->stack_p=stack;

    pid_t child_pid = clone(child_function, stack + stack_size, flags, (void*) parameters);
    if (child_pid == -1) {
        perror("clone");
        free(stack);
        exit(EXIT_FAILURE);
    }

    info_c->child_id=child_pid;

    return info_c;

}

/*Temporary test function to showcas the use of clone.h 
and how to wait the child with signal fd*/
int main() {
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

    const char* filepath = "./foo";
    parameter_clone* param = malloc(sizeof(parameter_clone));
    if (!param) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    param->filepath=filepath;
    param->args = (char* const[]) { "./foo","5", NULL };
    info_child* info = launch_process(STACK_SIZE,param,0);

    for (;;) {
        s = read(sfd, &fdsi, sizeof(fdsi));
        if (s != sizeof(fdsi)){
            perror("read");
            exit(2);
        } else if (fdsi.ssi_signo == SIGCHLD) {
            printf("Got SIGCHLD\n");
            printf("Processus parent : PID = %d, Fils = %d\n", getpid(), info->child_id);
            waitpid(info->child_id, NULL, 0);  // Attente de la fin du fils
            free(info->stack_p);
            break;
        } else {
            printf("Read unexpected signal\n");
        }
    }
    close(sfd);

    return 0;
}
