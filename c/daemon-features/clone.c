#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include "../include/clone.h"
#include <sys/signalfd.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include "../include/Errors.h"


int child_function(void *arg) {
    printf("Processus enfant : PID = %d, PPID = %d\n", getpid(), getppid());
    execve_parameter* params = (execve_parameter *)arg;

    int write_fd =params->pipe;

    // closing reading side
    close(write_fd - 1);
    
    if (dup2(params->error_file,STDOUT_FILENO)==-1) {
        perror("dup2");
    }

    if (execve((const char*) params->filepath, (char* const*) params->args, (char* const*) params->envp)==-1){
        printf("Something went wrong when opening executing program\n");
        perror("execve");
        int err = errno;
        write(write_fd, &err, sizeof(err));
        _exit(err);
    };
    
    exit(0);
}

//args and envp must be NULL terminated. It may lead to some issu and 
// it may be necessarie de append NULL to the argument given by the user
info_child* launch_process(int stack_size, execve_parameter* parameters, int flags){
    info_child* info_c = malloc(sizeof(info_child));
    if (!info_c){
        perror("malloc");
        return NULL;
    }

    void *stack = malloc(stack_size);
    if (!stack) {
        perror("malloc");
        free(info_c);
        return NULL;
    }

    info_c->stack_p=stack;

    int pipefd[2];
    if (pipe(pipefd) < 0) {
        perror("pipe"); exit(1);
    }

    // Marquer l'extrémité écriture CLOEXEC
    fcntl(pipefd[1], F_SETFD, FD_CLOEXEC);

    parameters->pipe=pipefd[1];

    pid_t child_pid = clone(child_function, stack + stack_size, SIGCHLD | flags, (void*) parameters);
    if (child_pid == -1) {
        printf("clone failed\n");
        fflush(stdout);
        perror("clone");
        free(stack);
        free(info_c);
        return NULL;
    }

    close(pipefd[1]);

    info_c->child_id=child_pid;
    info_c->pipe=pipefd[0];

    return info_c;

}

/*Temporary test function to showcas the use of clone.h 
and how to wait the child with signal fd*/
/*
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
    execve_parameter* param = malloc(sizeof(execve_parameter));
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
}*/
