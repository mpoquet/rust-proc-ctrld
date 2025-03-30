#ifndef __HEALTHCHECK_C__
#define __HEALTHCHECK_C__

#include <unistd.h>

#define HEALTHY 20
#define STARTING 10
#define UNHEATHY 5
#define RUNNING 1
#define TERMINATED 0

typedef struct s_health_status{
    int group_id; //0 for daemon
    int health_point;
    int state;
}health_status;

int get_health_status(int group_id);

void initialize_health_status(int group_id);

#endif
