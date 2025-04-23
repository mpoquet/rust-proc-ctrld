#include "../include/process_manager.h"
#include <stdlib.h>
#include <stdio.h>
#include "../include/Errors.h"

void free_manager(process_info** manager, int size){
    for(int i = 0; i<size; i++){
        free(manager[i]);
    }
    free(manager);
}

process_info** create_process_manager(int size){
    process_info** group = malloc(sizeof(process_info*)*size);
    if(group==NULL){
        perror("malloc");
        return NULL;
    }
    for(int i=0; i<size; i++){
        group[i]=malloc(sizeof(process_info));
        if(group[i]==NULL){
            perror("malloc");
            free_manager(group, i);
            return NULL;
        }
        group[i]->stack_p=NULL;
    }
    return group;
}

int search_process(int pid, int size, process_info** group){
    for(int i=0; i<size; i++){
        if (group[i]->child_id==pid){
            return i;
        }
    }
    return -1;
}

int manager_remove_process(int pid, process_info** manager, int size){
    for(int i=0; i<size; i++){
        if (manager[i]->child_id==pid){
            free(manager[i]->stack_p);
            manager[i]->stack_p=NULL;
            return 0;
        }
    }
    printf("Process %d not found", pid);
    return -1;

}

int manager_add_process(pid_t pid, process_info** manager, struct clone_parameters param, void* stack, int nb_process){
    if(nb_process<MAX_PROCESS){
        for (int j=0; j<MAX_PROCESS; j++){
            if(manager[j]->stack_p==NULL){
                manager[j]->child_id=pid;
                manager[j]->stack_p=stack;
                manager[j]->param=param;
                return 0;
            }
        }
        return -1;
    }else{
        printf("Cannot keep track of more process");
        return -1;
    }
}

