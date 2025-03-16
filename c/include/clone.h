#ifndef __CLONE_H__
#define __CLONE_H__

#include <unistd.h>

typedef struct parameter {
    char* filepath;
    char* const* args;
} parameter_clone;

typedef struct info {
    void * stack_p;
    pid_t child_id;
} info_child;
  
info_child* launch_process(int stack_size, parameter_clone* parameters, int flags);

#endif
