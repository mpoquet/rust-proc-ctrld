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
#include "../include/serialize_c.h"
#include <sys/inotify.h>
#include <sys/signalfd.h>

unsigned char buffer[512];

int try_connecting_to_daemon(int sock, struct sockaddr_in serv_addr){
    for (int i = 0; i < 5; i++) {
        if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == 0) {
            printf("Connection succeeded\n");
            break;
        }
        printf("Connection failed. Retrying...\n");
        sleep(1);
    }

    int size;

    if(read(sock, &size,sizeof(int))==-1){
        close(sock);
        return -1;
    };

    printf("Size received\n");
    fflush(stdout);

    if (recv(sock, buffer, size, MSG_WAITALL) != size) {
        perror("recv MSG_WAITALL");
        close(sock);
        exit(1);
    }

    printf("Mess received\n");
    fflush(stdout);

    int port = receive_TCPSocketListening_c(buffer,size);

    printf("Mess deserialized\n");
    fflush(stdout);

    printf("NumPort received : %d\n", port);

    if(htons(port)==serv_addr.sin_port){
        return 0;
    }

    perror("connect");
    return -1;

    return 0;


}

int try_launching_process(int sock, struct sockaddr_in serv_addr, command* com){

    unsigned char buf[128];

    struct buffer_info* info = send_command_to_demon_c(com);

    uint8_t* temp_buffer = malloc(sizeof(int) + info->size);
    if (!temp_buffer) {
        return -1;
    }

    // append size to data
    memcpy(temp_buffer, &info->size, sizeof(int));
    memcpy(temp_buffer + sizeof(int), info->buffer, info->size);

    // send data+size
    ssize_t sent = send(sock, temp_buffer, sizeof(int) + info->size, 0);
    free(temp_buffer);

    if (sent != sizeof(int) + info->size) {
        return -1;
    }
    
    int size;

    if(read(sock, &size,sizeof(int))==-1){
        close(sock);
        return -1;
    };

    printf("Size received : %d\n", size);
    fflush(stdout);

    // <- après avoir lu et converti net_size en size
    if (recv(sock, buf, size, MSG_WAITALL) != size) {
        perror("recv MSG_WAITALL");
        close(sock);
        exit(1);
    }

    int pid = receive_processlaunched_c(buf,size);

    printf("pid : %d\n", pid);

    if(pid<0){
        close(sock);
        return -1;
    }

    return 0;
}

int receiving_process_terminated(int sock, struct sockaddr_in serv_addr){

    unsigned char buf[128];
    
    int size;

    if(read(sock, &size,sizeof(int))==-1){
        close(sock);
        return -1;
    };

    printf("Size received\n");
    fflush(stdout);

    if (recv(sock, buf, size, MSG_WAITALL) != size) {
        perror("recv MSG_WAITALL");
        close(sock);
        exit(1);
    }

    printf("size : %d\n", size);
    fflush(stdout);

    struct process_terminated_info* info = receive_processterminated_c(buf,size);

    printf("pid : %d, error code : %d\n", info->pid, info->error_code);

    return 0;
}


int main(int argc, char** argv){
    /*launching daemon*/
    if(argc<2){
        printf("Usage : ./Regression_test -numPort");
        exit(1);
    }
    execve_parameter param_daemon;
    param_daemon.args = (char* const[]) { "./daemon",argv[1], NULL };
    param_daemon.envp = NULL;
    param_daemon.error_file = STDOUT_FILENO;
    param_daemon.filepath = "./daemon";
    launch_process(1024*1024, &param_daemon,0);

    sleep(2);

    /*Initialising communication socket*/

    int sock = 0;
    struct sockaddr_in serv_addr;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        close(sock);
        perror("socket");
        exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[1]));

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        close(sock);
        perror("inet_pton");
        exit(EXIT_FAILURE);
    }

    /*Start of tests*/

    assert(try_connecting_to_daemon(sock, serv_addr)==0);

    printf("TEST 1 succeeded, TCP connexion to the daemon is working\n");
    fflush(stdout);

    command* cmd = malloc(sizeof(command));
    cmd->args = (char* const[]) { "ls", "-l", NULL };
    cmd->args_size=2;
    cmd->envp=NULL;
    cmd->envp_size=0;
    cmd->flags=0;
    cmd->path="/bin/ls";
    cmd->stack_size=512*512;
    cmd->to_watch=NULL;
    cmd->to_watch_size=0;

    assert(try_launching_process(sock,serv_addr,cmd)==0);

    printf("TEST 2 succeeded, remotely executing programs works\n");
    fflush(stdout);

    assert(receiving_process_terminated(sock,serv_addr)==0);

    printf("TEST 3 succeeded, getting exit status of termiated programms\n");
    fflush(stdout);

    // struct surveillance* to_watch= malloc(sizeof(struct surveillance)*1); //Fois 1 parce que la y'a que une élément
    // to_watch->event=INOTIFY;
    // struct inotify_parameters* I_param = malloc(sizeof(struct inotify_parameters));
    // I_param->root_paths="/home/etc";
    // I_param->size=0;
    // I_param->mask=IN_MODIFY;
    // to_watch->ptr_event=(void*)I_param;
    // cmd = malloc(sizeof(command));
    // cmd->args = NULL;
    // cmd->args_size=0;
    // cmd->envp=NULL;
    // cmd->envp_size=0;
    // cmd->flags=0;
    // cmd->path=NULL;
    // cmd->stack_size=0;
    // cmd->to_watch=to_watch;
    // cmd->to_watch_size=1;
    close(sock);
}