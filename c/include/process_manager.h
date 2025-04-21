#ifndef __PROCESS_MANAGER_C__
#define __PROCESS_MANAGER_C__

#include <unistd.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include "./Network.h"

#define MAX_PROCESS 256

typedef struct s_process_info{
    void * stack_p;
    pid_t child_id;
    struct clone_parameters param;
}process_info;

process_info** create_process_manager(int size);

int manager_remove_process(int pid, process_info** manager, int size);

int manager_add_process(pid_t pid, process_info** manager, struct clone_parameters param, void* stack, int nb_process);

#endif