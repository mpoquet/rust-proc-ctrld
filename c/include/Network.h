#ifndef __NETWORK_H__
#define __NETWORK_H__

#include <unistd.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

struct tcp_socket{
    uint8_t destport;
};

struct buffer_info{
    void *buffer;
    size_t size;
};

struct process_terminated_info{
    int32_t pid;
    uint32_t errno;
};

struct socket_info{
    int port;
    struct sockaddr_in* address;
    int sockfd;
};

struct inotify_parameters{
    char *root_paths;
    enum InotifyEvent* i_events;
    uint32_t size;
};

enum SurveillanceEventType{
    INOTIFY,
    WATCH_SOCKET,
};

struct surveillance {
    enum SurveillanceEventType event;
    void *ptr_event;
};

typedef struct s_command{ //template for deserialized struct which contained all the info for all possible command. CAN BE MODIFIED
    char *path;
    size_t args_size; 
    size_t envp_size;
    uint32_t flags;
    uint32_t stack_size;
    struct surveillance *to_watch; //pointer to array of structs (can be null)
    size_t to_watch_size;
    char **envp;
    char **args;
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


struct socket_info* establish_connection(int port); //exemple of function i want for the daemon 

int accept_new_connexion(struct socket_info* info);

void send_command(command *cmd);

int read_socket(int serveur_fd, char* buffer);

enum Event receive_message_from_user(void *buffer, size_t size);

static command* receive_command(void *buffer, size_t size);    //exemple of function i want for the daemon; Return NULL in case of failure 

#endif 