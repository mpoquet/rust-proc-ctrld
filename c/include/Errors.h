#ifndef __ERRORS_C__
#define __ERRORS_C__

#include <unistd.h>
#include "./Network.h"

enum error_type{
    CHILD_EXITED,
    CHILD_SIGNALED,
    CLONE_ERR,
    FILE_CREATION,
    GROUP_FULL,
    GROUP_PROCESS_FULL,
};

struct child_err{
    int pid;
    command* com;
    int group_id;
};

struct clone_err{
    int group_id;
    command* com;
};

struct group_full{
    int group_id;
    command* com;
};

struct group_process_full{
    int group_id;
    command* com;
    int pid;
};

struct file_err{
    char* filepath;
    int group_id;
    int pid;
};

int initialize_error_file(const char* filepath);

int send_error(enum error_type type, void* err_data);

#endif
