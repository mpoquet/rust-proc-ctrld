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

#include "./include/clone.h"
#include "./include/healthcheck.h"
#include "./include/Errors.h"


#define MAX_PROCESS_GROUPS 128

int current_groups_id[MAX_PROCESS_GROUPS]={0};
int last_group_id;

int main(){
    //Commenting to simplify testing
    /*
    // change to the "/" directory
    int nochdir = 0;

    // redirect standard input, output and error to /dev/null
    // this is equivalent to "closing the file descriptors"
    int noclose = 0;

    // glibc call to daemonize this process without a double fork
    if(daemon(nochdir, noclose))
        perror("daemon");

    // sleep to give us time to verify the demon is working. To verify, execute :
    // pgrep daemon-core
    // ps -p pid -o "user pid ppid pgid sid tty command"
    // lsof -p pid

    sleep(60);
    */

    initialize_health_status(0);

    initialize_error_data(0,"Demon_errors_trace.txt");

    int epollfd = epoll_create1(0);


    return 0;

}