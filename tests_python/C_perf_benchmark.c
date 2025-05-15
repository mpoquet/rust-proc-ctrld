#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/signalfd.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>


typedef struct s_execve_paramter {
    char* filepath;
    char* const* args;
    int error_file;
    char* const* envp;
} execve_parameter;

typedef struct info {
    void * stack_p;
    pid_t child_id;
} info_child;

int child_function(void *arg) {
    printf("Processus enfant : PID = %d, PPID = %d\n", getpid(), getppid());
    execve_parameter* params = (execve_parameter *)arg;
    
    if (dup2(params->error_file,STDOUT_FILENO)==-1) {
        perror("dup2");
    }

    if (execve((const char*) params->filepath, (char* const*) params->args, (char* const*) params->envp)==-1){
        printf("Something went wrong when opening executing program\n");
        perror("execve");
        _exit(1);
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

    pid_t child_pid = clone(child_function, stack + stack_size, SIGCHLD | flags, (void*) parameters);
    if (child_pid == -1) {
        printf("clone failed\n");
        fflush(stdout);
        perror("clone");
        free(stack);
        free(info_c);
        return NULL;
    }

    info_c->child_id=child_pid;

    return info_c;

}

int main(int argc, char** argv){
    execve_parameter* p = malloc(sizeof(execve_parameter));
    if(p==NULL){
        return 1;
    }
    p->filepath="/bin/echo";
    p->args=(char* const[]) { "echo", "bonjour", NULL };
    p->envp=NULL;
    p->error_file=STDOUT_FILENO;

    struct timespec start, end;
    double elapsed_time;

    if (clock_gettime(CLOCK_MONOTONIC, &start) == -1) {
        perror("clock_gettime failed");
        return 1;
    }

    for(int i =0; i<100; i++){
        info_child* res = launch_process(512*512,p,0);
    }

    for(int i =0; i<100; i++){
        waitpid(0, NULL, 0);
    }

    if (clock_gettime(CLOCK_MONOTONIC, &end) == -1) {
        perror("clock_gettime failed");
        return 1;
    }

    elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1000000000.0;

    printf("Temps écoulé : %f secondes\n", elapsed_time);

    return 0;

}