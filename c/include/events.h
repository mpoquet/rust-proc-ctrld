#ifndef __EVENTS_H__
#define __EVENTS_H__

#include <unistd.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include "./Network.h"
#include "./clone.h"
#include "./process_manager.h"

enum eventType{
    SIGNALFD,
    INOTIFYFD,
    SOCK_CONNEXION,
    SOCK_MESSAGE,
};

struct clone_parameters{
    int stack_size;
    char *pathname;
    int flags;
    char* const* args;
    char* const* envp;
};

typedef struct {
    int fd;
    enum eventType type;

} event_data_t;

typedef struct {
    int fd;
    enum eventType type;
    struct socket_info* sock_info;
} event_data_sock;

typedef struct {
    int fd;
    enum eventType type;
    int size;
} event_data_Inotify_size;

typedef struct s_process_info process_info;

void process_surveillance_requests(command* com, int InotifyFd, int epollfd);

info_child* handle_clone_event(struct clone_parameters* param, int errorfd);

int add_event_signalFd(int fd, int epollfd);

int add_event_inotifyFd(int fd, int epollfd, int size);

void* handle_signalfd_event(int fd, process_info** manager, int size);

void handle_inotify_event(int fd, int size);

struct clone_parameters* extract_clone_parameters(command* com);

#endif