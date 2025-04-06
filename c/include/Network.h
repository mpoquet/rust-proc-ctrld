#ifndef __NETWORK_C__
#define __NETWORK_C__

#include <unistd.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/types.h>

enum NetworkEventType{
    INOTIFY, //template for multiple type to recognize the type of messages 
    CLONE,
    WATCH_SOCKET,
};

struct clone_parameters{
    int stack_size;
    char *pathname;
    int flags;
    char* const* args;
    char* const* envp;
    int group_id;
};

struct inotify_parameters{
    char *pathname;
    int flags;
    int size_target; //once the file reach that size a notification will be sent
};

typedef struct s_command{ //template for deserialized struct which contained all the info for all possible command. CAN BE MODIFIED
    char* command_name;
    char* arg1;
    char* arg2;
    char* arg3;
    enum NetworkEventType type;
}command;

int establish_connection(int port); //exemple of function i want for the daemon 

command* receive_message(); //exemple of function i want for the daemon; Return NULL in case of failure 

void send_message(); //exemple of function i want for the daemon 

struct inotify_parameters* extract_inotify_info(command* com);

struct clone_parameters* extract_clone_info(command* com);

#endif