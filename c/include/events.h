#ifndef __EVENTS_H__
#define __EVENTS_H__

#include <unistd.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include "./serialize_c.h"  
#include "./Network.h"
#include "./clone.h"
#include "./process_manager.h"

enum eventType{
    SIGNALFD,
    INOTIFYFD,
    SOCK_CONNEXION,
    SOCK_MESSAGE,
    EXECVE,
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

typedef struct {
    int fd;
    enum eventType type;
    int pid;
    char* path;
} event_data_pipe_execve;

typedef struct s_process_info process_info;

int add_event_pipeExecve(int fd, int epollfd, int pid, char* path);

void process_surveillance_requests(command* com, int InotifyFd, int epollfd, int communication_socket);

void process_command_request(process_info **process_manager, int* nb_process, int com_sock, command* com, int epollfd );

info_child* handle_clone_event(struct clone_parameters* param, int errorfd);

int add_event_signalFd(int fd, int epollfd);

int add_event_inotifyFd(int fd, int epollfd, int size);

struct buffer_info* handle_signalfd_event(int fd, process_info** manager, int* size);

void handle_inotify_event(int fd, int size, int com_sock);

struct clone_parameters* extract_clone_parameters(command* com);

#endif