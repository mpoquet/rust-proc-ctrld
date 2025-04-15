#ifndef __GROUP_MANAGER_C__
#define __GROUP_MANAGER_C__

#include <unistd.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include "./Network.h"

#define MAX_PROCESS_BY_GROUPS 8

struct process_info{
    void * stack_p;
    pid_t child_id;
    command proc_command;
};

typedef struct s_group_info{
    int health_points;
    int nb_process;
    struct process_info group_process[MAX_PROCESS_BY_GROUPS];
    int err_file;
    int group_id;
    uint8_t active;
}group_info;

group_info** create_group_manager(int size);

int manager_add_group(int group_id, group_info** manager, int size , char* error_file_path);

int manager_remove_group(int group_id, group_info** manager,int size);

int manager_remove_process(int pid, int group_id, group_info** manager, int size);

int manager_add_process(pid_t pid, group_info* manager, command com, void* stack);

#endif