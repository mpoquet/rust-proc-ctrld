#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include "../include/clone.h"

#define STACK_SIZE 1024*1024  // Taille de la pile du fils


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
/*
int main() {

    // struct sigaction sa;
    // sigemptyset(&sa.sa_mask);
    // sa.sa_flags = SA_RESTART;
    // sa.sa_handler = sigio_handler;
    // if (sigaction(SIGIO, &sa, NULL) == -1) {
    //     fprintf(stderr, "sigaction");
    // }


    const char* filepath = "./foo";
    parameter_clone* param = malloc(sizeof(parameter_clone));
    if (!param) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    param->filepath=filepath;
    param->args = (char* const[]) { "./foo","5", NULL };
    info_child* info = launch_process(STACK_SIZE,param,SIGCHLD);

    printf("Processus parent : PID = %d, Fils = %d\n", getpid(), info->child_id);
    waitpid(info->child_id, NULL, 0);  // Attente de la fin du fils
    free(info->stack_p);

    return 0;
}*/
