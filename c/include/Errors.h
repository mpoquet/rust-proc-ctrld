#ifndef __ERRORS_C__
#define __ERRORS_C__

#include <unistd.h>

#define ACTIVE 1
#define UNACTIVE 0

typedef struct s_error{
    int* pfd; //pipe fd in read mode to read errors message of the group
    int fd; //fd of error file that keeps track of all errors
    u_int8_t group_id;
    u_int8_t state;
} error_data;

void initialize_error_data(int group_id, const char* filepath);

void delete_error_data(int group_id);

#endif
