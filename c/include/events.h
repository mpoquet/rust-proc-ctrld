#ifndef __EVENTS_C__
#define __EVENTS_C__

#include <unistd.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/types.h>

typedef struct {
    int fd;
    uint32_t type; // INOTIFYFD ou SIGNALFD
} event_data_t;

#endif