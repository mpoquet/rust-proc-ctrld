#ifndef __EVENTS_C__
#define __EVENTS_C__

#include <unistd.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/types.h>

enum eventType{
    SIGNALFD,
    INOTIFYFD,
    SOCKFD,
};

typedef struct {
    int fd;
    enum eventType type;
    int group_id;
} event_data_t;

int add_event_signalFd(int fd, int epollfd);

int add_event_inotifyFd(int fd, int epollfd);

void handle_signalfd_event(int fd);

void handle_inotify_event(int fd);

#endif