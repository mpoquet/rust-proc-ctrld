#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include "../include/Errors.h"


int initialize_error_file(const char* filepath){
    printf("initializing file");
    int fd = open(filepath, O_WRONLY | O_CREAT | O_APPEND, S_IRWXU); //The permission are temporary and may be modified
    if(fd==-1){
        perror("open");
        return -1;
    }
    return fd;
}

int send_error(enum error_type type, void* err_data){
    return 0;
}

