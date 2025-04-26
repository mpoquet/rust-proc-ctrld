#ifndef __NETWORK_H__
#define __NETWORK_H__

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "demon_builder.h"
#include "demon_verifier.h"
#include "demon_reader.h"

struct tcp_socket{
    uint8_t destport;
};

struct buffer_info{
    void *buffer;
    size_t size;
};

struct process_terminated_info{
    int32_t pid;
    uint32_t error_code;
};

struct socket_info{
    int port;
    struct sockaddr_in address;
    int sockfd;
};

//event = constante definie par flatbuffers de la forme : demon_InotifyEvent_modification
struct inotify_parameters{
    char *root_paths;
    demon_InotifyEvent_enum_t* i_events; 
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

int32_t receive_kill(void *buffer, size_t size);

struct socket_info* establish_connection(int port); //exemple of function i want for the daemon 

int accept_new_connexion(struct socket_info* info);

void send_command(command *cmd);

struct buffer_info* send_processlaunched_to_user(int32_t pid);

int32_t receive_processlaunched(void *buffer, size_t size);

uint16_t receive_TCPSocket(void *buffer, size_t size);

struct buffer_info* send_tcpsocketlistening_to_user(uint16_t port);

struct buffer_info* send_childcreationerror_to_user(uint32_t error_code);

void serialize_command(flatcc_builder_t *B, command *cmd);

struct buffer_info* send_command_to_demon(command *cmd);

int send_message(int socket, void* buffer, int size);

struct buffer_info* send_processterminated_to_user(int32_t pid, uint32_t errno);

int read_socket(int serveur_fd, void* buffer, int size);

enum Event receive_message_from_user(void *buffer);

command* receive_command(void *buffer, size_t size);    //exemple of function i want for the daemon; Return NULL in case of failure 

#endif 