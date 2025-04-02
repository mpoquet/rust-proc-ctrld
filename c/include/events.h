#ifndef __EVENTS_C__
#define __EVENTS_C__

#include <unistd.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/types.h>

typedef struct {
    int fd;
    uint8_t type; // INOTIFYFD or SIGNALFD or ERRORFD
    int group_id;
} event_data_t;

#endif