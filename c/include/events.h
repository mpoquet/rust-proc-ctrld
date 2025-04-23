#ifndef __EVENTS_C__
#define __EVENTS_C__

#include <unistd.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include "./Network.h"
#include "../include/process_manager.h"

enum eventType{
    SIGNALFD,
    INOTIFYFD,
    SOCKFD,
};

enum InotifyEvent{
    MODIFY,
    CREATE,
    DELETE,
    ACCESS,
}

typedef struct {
    int fd;
    enum eventType type;
    int group_id;
} event_data_t;

info_child* handle_clone_event(struct clone_parameters* param, int errorfd);

int add_event_signalFd(int fd, int epollfd);

int add_event_inotifyFd(int fd, int epollfd);

int handle_signalfd_event(int fd, process_info** manager, int size);

void handle_inotify_event(int fd);

#endif