#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include "../include/Errors.h"


int initialize_error_file(const char* filepath){
    printf("initializing file");
    int fd = open(filepath, O_APPEND | O_CREAT , S_IRWXU); //The permission are temporary and may be modified
    if(fd==-1){
        perror("open");
        return -1;
    }
    return fd;
}

int send_error(enum error_type type, void* err_data){
    return 0;
}

/*Initialize all the info necessary to handle error messages. A tube to communicate with the childs
and a file to store error information.
void initialize_error_data(int group_id, const char* filepath){
    for(int i=0;i<128;i++){
        if (i>=nb_groups || Error_data[i].state==UNACTIVE){
            initialize_error_file(Error_data[i],filepath);
            Error_data[i].group_id=group_id;
            Error_data[i].state=ACTIVE;
        }
    }
    nb_groups++;
}

void delete_error_data(int group_id){
    for (int i=0; i<128; i++){
        if(Error_data[i].group_id==group_id){
            Error_data[i].state=UNACTIVE;
            break;
        }
    }
    nb_groups--;
}*/

