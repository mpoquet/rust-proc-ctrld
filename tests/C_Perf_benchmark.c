#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include "../c/include/clone.h"
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
#include <time.h>

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