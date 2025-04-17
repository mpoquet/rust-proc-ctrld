#include "../include/group_manager.h"
#include <stdlib.h>
#include <stdio.h>
#include "../include/Errors.h"

void free_manager(group_info** manager, int size){
    for(int i = 0; i<size; i++){
        free(manager[i]);
    }
    free(manager);
}

group_info** create_group_manager(int size){
    group_info** group = malloc(sizeof(group_info*)*size);
    if(group==NULL){
        perror("malloc");
        return NULL;
    }
    for(int i=0; i<size; i++){
        group[i]=malloc(sizeof(group_info));
        if(group[i]==NULL){
            perror("malloc");
            free_manager(group, i);
            return NULL;
        }
        group[i]->active=0;
        group[i]->health_points=20;
        group[i]->nb_process=0;
        for(int j =0; j<MAX_PROCESS_BY_GROUPS; j++){
            group[i]->group_process[j].child_id=-1;
        }
    }
    return group;
}

int search_group(int num_group, int size, group_info** group){
    for(int i=0; i<size; i++){
        if (group[i]->group_id==num_group){
            return i;
        }
    }
    return -1;
}

int manager_add_group(int group_id, group_info** manager, int size, char* error_file_path){
    int num;
    if((num=search_group(group_id,size,manager))!=-1){
        return num;
    }
    for(int i=0; i<size; i++){
        if (manager[i]->active==0){
            if((manager[i]->err_file=initialize_error_file(error_file_path))==-1){
                printf("An error has occured when trying to create de error file : %s, for the group : %d\n", error_file_path, group_id);
                return -1;
            }
            manager[i]->group_id=group_id;
            manager[i]->active=1;
            return i;
        }
    }
    printf("Not enough space to create new group\n");
    return -1;
}

int manager_remove_group(int group_id, group_info** manager,int size){
    for(int i=0; i<size; i++){
        if (manager[i]->group_id==group_id){
            manager[i]->active=0;
            manager[i]->nb_process=0;
            manager[i]->health_points=20;
            return 0;
        }
    }
    printf("group %d, not found\n",group_id);
    return -1;
}

int manager_remove_process(int pid, int group_id, group_info** manager, int size){
    for(int i=0; i<size; i++){
        if (manager[i]->group_id==group_id){
            for (int j=0; j<MAX_PROCESS_BY_GROUPS; j++){
                if (manager[i]->group_process[j].child_id==pid){
                    free(manager[i]->group_process[j].stack_p);
                    manager[i]->group_process[j].child_id=-1;
                    manager[i]->nb_process--;
                    return 0;
                }
            }
            printf("The process %d does not exist in the group %d\n", pid, group_id);
            return -1;
        }
    }
    printf("group %d, not found\n",group_id);
    return -1;

}

int manager_add_process(pid_t pid, group_info* manager, command com, void* stack){
    if(manager->nb_process<MAX_PROCESS_BY_GROUPS){
        for (int j=0; j<MAX_PROCESS_BY_GROUPS; j++){
            if(manager->group_process[j].child_id!=-1){
                manager->group_process[j].child_id=pid;
                manager->group_process[j].proc_command=com;
                manager->group_process[j].stack_p=stack;
                manager->nb_process++;
                return 0;
            }
        }
        return -1;
    }else{
        printf("The group %d is allready full\n", manager->group_id);
        return -1;
    }
}

