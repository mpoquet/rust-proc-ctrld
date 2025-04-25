#ifndef __ERRORS_C__
#define __ERRORS_C__

#include <unistd.h>

enum error_type{
    CHILD_EXITED,
    CHILD_SIGNALED,
    CLONE_ERR,
};

// struct child_err{
//     int pid;
//     command com;
// };

// struct clone_err{
//     command* com;
// };

int initialize_error_file(const char* filepath);

int send_error(enum error_type type, void* err_data);

#endif
