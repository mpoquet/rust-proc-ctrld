#ifndef __ERRORS_C__
#define __ERRORS_C__

#include <unistd.h>
#include "./Network.h"

enum error_type{
    CHILD_EXITED,
    CHILD_SIGNALED,
    CLONE_ERR,
    FILE_CREATION,
};

struct child_err{
    int pid;
    command* com;
    int group_id;
};

struct clone_err{
    
};

struct file_err{
    char* filepath;
    int group_id;
    int pid;
};

int initialize_error_file(const char* filepath);

int send_error(enum error_type type, void* err_data);

#endif
