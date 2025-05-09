#ifndef SERIALIZE_H
#define SERIALIZE_H

#include <cstdint>  // for uint32_t, int32_t, etc.
#include <sys/epoll.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>  // for sockaddr_in
#include <netinet/in.h>  // for sockaddr_in
#include <cstddef> 

struct tcp_socket{
    uint32_t destport;
};

struct buffer_info{
    uint8_t *buffer;
    int size;
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

enum InotifyEvent{
    MODIFIED,
    CREATED,
    SIZE_REACHED,
    DELETED,
    ACCESSED,
};

//event = constante definie par flatbuffers de la forme : demon_InotifyEvent_modification
struct inotify_parameters{
    char *root_paths;
    int mask;
    uint32_t size;
    uint32_t size_limit;
};

enum SurveillanceEventType{
    INOTIFY,
    WATCH_SOCKET,
};

struct surveillance {
    enum SurveillanceEventType event;
    void *ptr_event;
};

struct InotifyPathUpdated{
    char* path;
    enum InotifyEvent *event;
    int size;
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
    NONE = -1,
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

buffer_info* send_command_to_demon(command *cmd);
buffer_info* send_kill_to_demon(int32_t pid);
command* receive_command(uint8_t *buffer, int size);
int32_t receive_kill(uint8_t *buffer, int size);

Event receive_message_from_user(uint8_t *buffer, int size);

buffer_info* send_processlaunched_to_user(int32_t pid);
buffer_info* send_childcreationerror_to_user(uint32_t error_code);
buffer_info* send_processterminated_to_user(int32_t pid, uint32_t error_code);
buffer_info* send_tcpsocketlistening_to_user(uint16_t port);
buffer_info* send_inotifypathupdated_to_user(InotifyPathUpdated *inotify) 

int32_t receive_processlaunched(uint8_t *buffer, int size);
int32_t receive_childcreationerror(uint8_t *buffer, int size);
process_terminated_info* receive_processterminated(uint8_t *buffer, int size);
uint16_t receive_TCPSocketListening(uint8_t *buffer, int size);
InotifyPathUpdated* receive_inotifypathupdated(uint8_t *buffer, int size);

Event receive_message_from_demon(uint8_t *buffer, int size);


#endif // SERIALIZE_H