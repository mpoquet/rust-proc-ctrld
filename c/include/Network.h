#ifndef __NETWORK_C__
#define __NETWORK_C__

#include <unistd.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/types.h>

enum NetworkEventType{
    INSTRUCTION, //template for multiple type to recognize the type of messages 
};

typedef struct s_command{ //template for deserialized struct. CAN BE MODIFIED
    char* command_name;
    char* arg1;
    char* arg2;
    char* arg3;
    enum NetworkEventType type;
}command;

int establish_connection(int port); //exemple of function i want for the daemon 

command receive_message(); //exemple of function i want for the daemon 

void send_message(); //exemple of function i want for the daemon 

#endif