#define _GNU_SOURCE
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/signalfd.h>
#include "../include/Errors.h"

error_data Error_data[128];
int nb_groups=0;

/*Returns a signalfd catching I/O signals to the file created
We use signalf + fcntl beacause we want to know who wrote into the file
without forcing on to developper a specific scheme they need to follow when
sending errors.*/

void initialize_error_data(int group_id, const char* filepath){
    for(int i=0;i<128;i++){
        if (i>=nb_groups || Error_data[i].group_id==group_id){
            initialize_error_file(Error_data[i],filepath);
            Error_data[i].group_id=group_id;
            initialize_error_channel(Error_data[i]);
        }
    }
    nb_groups++;
}

void initialize_error_file(error_data data, const char* filepath){
    int fd = open(filepath, O_APPEND | O_CREAT, S_IRWXU);
    if(fd==-1){
        perror("open");
        exit(1);
    }
    data.fd=fd;
}

/*Return NULL if creation of tube failed*/
void initialize_error_channel(error_data data){
    int pipefd[2];

    if(pipe2(pipefd,O_DIRECT)==-1){ //O_DIRETC to interpret each message individually 
        perror("pipe");
        exit(1);
    }

    data.pfd=pipefd;
}

