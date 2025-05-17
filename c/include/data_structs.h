#ifndef DATA_STRUCTS_H
#define DATA_STRUCTS_H

#ifdef __cplusplus
    #include <cstdint>  // C++ version
    #include <cstddef>  // C++ version
    #include <cstdbool> // C++ version
#else
    #include <stdint.h>  // C version
    #include <stddef.h>  // C version
    #include <stdbool.h> // C version
#endif


struct tcp_socket{
    uint32_t destport;
};

struct buffer_info{
    uint8_t *buffer;
    int size;
};

struct execve_info{
    int32_t pid;
    char *command_name;
    bool success;
};

struct process_terminated_info{
    int32_t pid;
    uint32_t error_code;
};

enum SocketState{
    SOCKET_UNKNOWN,
    SOCKET_LISTENING,
    SOCKET_CREATED,
};

struct socket_watch_info{
    int32_t port;
    enum SocketState state;
};

enum InotifyEvent{
    MODIFIED,
    CREATED,
    SIZE_REACHED,
    DELETED,
    ACCESSED,
};

struct inotify_parameters{
    char *root_paths;
    int mask;
    uint32_t size;};

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
    enum InotifyEvent event;
    uint32_t size;
    uint32_t size_limit;

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
    INOTIFY_WATCH_LIST_UPDATED,
    SOCKET_WATCHED,
    SOCKET_WATCH_TERMINATED,
};


#endif // DATA_STRUCTS_H