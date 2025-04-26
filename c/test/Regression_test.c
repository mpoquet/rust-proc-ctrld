#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include "../include/clone.h"
#include <assert.h>
#include "../include/Network.h"

int try_connecting_to_daemon(int sock, struct sockaddr_in serv_addr){
    for (int i = 0; i < 5; i++) {
        if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == 0) {
            printf("Connection succeeded\n");
            return 0;
        }
        printf("Connection failed. Retrying...\n");
        sleep(1);
    }
    perror("connect");
    return -1;
}

int try_launching_process(int sock, struct sockaddr_in serv_addr, command* com){

    struct buffer_info* buffer = send_command_to_demon(com);

    if(send(sock, (void*)buffer, sizeof(struct buffer_info),0)==-1){
        close(sock);
        return -1;
    };

    struct buffer_info* res;

    if(read(sock, res,sizeof(struct buffer_info))==-1){
        close(sock);
        return -1;
    };

    int pid = receive_processlaunched(res,sizeof(struct buffer_info));

    printf("pid : %d", pid);

    if(pid<0){
        close(sock);
        return -1;
    }

    close(sock);
    return 0;
}


int main(int argc, char** argv){
    /*launching daemon*/
    if(argc<2){
        printf("Usage : ./Regression_test -numPort");
        exit(1);
    }
    execve_parameter param_daemon;
    param_daemon.args = (char* const[]) { "../bin/daemon",argv[1], NULL };
    param_daemon.envp = NULL;
    param_daemon.error_file = STDOUT_FILENO;
    param_daemon.filepath = "../bin/daemon";
    launch_process(1024*1024, &param_daemon,0);

    sleep(2);

    /*Initialising communication socket*/

    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        close(sock);
        perror("socket");
        exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[1]));

    printf("%d\n",atoi(argv[1]));

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        close(sock);
        perror("inet_pton");
        exit(EXIT_FAILURE);
    }

    /*Start of tests*/

    assert(try_connecting_to_daemon(sock, serv_addr)==0);

    command* cmd = malloc(sizeof(command));
    cmd->args = (char* const[]) { "ls", NULL };
    cmd->args_size=1;
    cmd->envp=NULL;
    cmd->envp_size=0;
    cmd->flags=0;
    cmd->path="ls";
    cmd->stack_size=512*512;
    cmd->to_watch=NULL;
    cmd->to_watch_size=0;

    assert(try_launching_process(sock,serv_addr,cmd)==0);

}