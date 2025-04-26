#ifndef __PROCESS_MANAGER_H__
#define __PROCESS_MANAGER_H__

#include <unistd.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include "./Network.h"

#define MAX_PROCESS 256

typedef struct s_process_info{
    void * stack_p;
    pid_t child_id;
    int err_file;
}process_info;

void free_manager(process_info** manager, int size);

process_info** create_process_manager(int size);

int search_process(int pid, int size, process_info** group);

int manager_remove_process(int pid, process_info** manager, int size);

int manager_add_process(pid_t pid, process_info** manager, int err_file, void* stack, int nb_process);

#endif