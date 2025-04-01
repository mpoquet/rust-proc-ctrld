#ifndef __ERRORS_C__
#define __ERRORS_C__

#include <unistd.h>

typedef struct s_error{
    int group_id;
    int fd; //fd of error file that keeps track of all errors
    int* pfd; //pipe fd in read mode to read errors message of the group 
} error_data;

void initialize_error_channel(error_data data);

void initialize_error_file(error_data data, const char* filepath);

#endif
