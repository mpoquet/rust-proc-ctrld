#include "../include/healthcheck.h"
#include <stdlib.h>
#include <stdio.h>

health_status* groups_health[128]; //contains health status for each group of programs
int nb_groups = 0;

/*Returns NULL if nothing found*/
health_status* get_group_health_stat(int group_id){
    for (int i=0; i<128; i++){
        if(groups_health[i]->group_id==group_id){
            return groups_health[i];
        }
    }
    printf("Error : Searching for health stats for group %d wich does not exist", group_id);
    return NULL;
}

int get_health_status(int group_id){
    health_status* status = get_group_health_stat(group_id);
    if (status->health_point>=HEALTHY){
        return HEALTHY;
    } else if (status->health_point<=UNHEATHY)
    {
        return UNHEATHY;
    }else{
        return STARTING;
    }
}

void add_group_health_status(health_status* status){
    for (int i=0; i<128; i++){
        if(i>=nb_groups || groups_health[i]->state==TERMINATED ){
            groups_health[i]=status;
            break;
        }
    }
    nb_groups++;
}

void delete_group_health_status(int group_id){
    for (int i=0; i<128; i++){
        if(groups_health[i]->group_id==group_id){
            groups_health[i]->state=TERMINATED;
            break;
        }
    }
    nb_groups--;
}

void initialize_health_status(int group_id){
    health_status* status = malloc(sizeof(health_status));
    if(!status){
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    status->group_id=group_id;
    status->health_point=STARTING;
    status->state=RUNNING;
    add_group_health_status(status);
}