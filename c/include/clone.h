#ifndef __CLONE_H__
#define __CLONE_H__

#include <unistd.h>

typedef struct s_execve_paramter {
    char* filepath;
    char* const* args;
    int error_file;
    char* const* envp;
    int pipe;
} execve_parameter;

typedef struct info {
    void * stack_p;
    pid_t child_id;
    int pipe;
} info_child;
  
info_child* launch_process(int stack_size, execve_parameter* parameters, int flags);

#endif
