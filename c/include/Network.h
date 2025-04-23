#ifndef __NETWORK_C__
#define __NETWORK_C__

#include <unistd.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include "./events.h"

struct tcp_socket{
    uint8_t destport;
};

struct inotify_parameters{
    char *root_paths;
    InotifyEvent* i_events;
    uint32_t size = 0;
};

enum SurveillanceEventType{
    INOTIFY,
    WATCH_SOCKET,
};

struct surveillance {
    SurveillanceEventType event;
    void *ptr_event;
};

typedef struct s_command{ //template for deserialized struct which contained all the info for all possible command. CAN BE MODIFIED
    char *path;
    char *args[];
    size_t args_size;
    char *envp[];
    size_t envp_size;
    uint32_t flags;
    uint32_t stack_size;
    struct surveillance *to_watch; //pointer to array of structs (can be null)
    size_t to_watch_size;
}command;


enum Event {
    RUN_COMMAND,
    KILL_PROCESS,
    ESTABLISH_TCP_CONNECTION,
    ESTABLISH_UNIX_CONNECTION,
    PROCESS_LAUNCHED,
    CHILD_CREATION_ERROR,
    PROCESS_TERMINATED,
    TCP_SOCKET_LISTENING,
    INOTIFY_PATH_UPDATED,
};


int establish_connection(int port); //exemple of function i want for the daemon 

void send_command(command *cmd);

static struct command* receive_command(void *buffer, size_t size);    //exemple of function i want for the daemon; Return NULL in case of failure 

#endif