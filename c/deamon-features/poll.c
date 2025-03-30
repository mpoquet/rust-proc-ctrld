#include <poll.h>
#include <fcntl.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/inotify.h>

static void displayInotifyEvent(struct inotify_event *i)
{
    printf("    wd =%2d; ", i->wd);
    if (i->cookie > 0)
        printf("cookie =%4d; ", i->cookie);

    printf("mask = ");
    if (i->mask & IN_ACCESS)        printf("IN_ACCESS %s", i->name);
    if (i->mask & IN_ATTRIB)        printf("IN_ATTRIB ");
    if (i->mask & IN_CLOSE_NOWRITE) printf("IN_CLOSE_NOWRITE ");
    if (i->mask & IN_CLOSE_WRITE)   printf("IN_CLOSE_WRITE ");
    if (i->mask & IN_CREATE)        printf("IN_CREATE ");
    if (i->mask & IN_DELETE)        printf("IN_DELETE ");
    if (i->mask & IN_DELETE_SELF)   printf("IN_DELETE_SELF ");
    if (i->mask & IN_IGNORED)      printf("IN_IGNORED ");
    if (i->mask & IN_ISDIR)      printf("IN_ISDIR ");
    if (i->mask & IN_MODIFY) {
        printf("IN_MODIFY ");
    }        
    if (i->mask & IN_MOVE_SELF)  printf("IN_MOVE_SELF ");
    if (i->mask & IN_MOVED_FROM)    printf("IN_MOVED_FROM ");
    if (i->mask & IN_MOVED_TO)    printf("IN_MOVED_TO ");
    if (i->mask & IN_OPEN)        printf("IN_OPEN ");
    if (i->mask & IN_Q_OVERFLOW)    printf("IN_Q_OVERFLOW ");
    if (i->mask & IN_UNMOUNT)      printf("IN_UNMOUNT ");
    printf("\n");

    if (i->len > 0)
        printf("        name = %s\n", i->name);
}

void handle_inotify_event(int fd){
    char buf[4096];
    ssize_t s = read(fd, buf, sizeof(buf));
    printf("    read %zd bytes: %.*s\n",
        s, (int) s, buf);
    for(char *p = buf; p<buf+s;){
        struct inotify_event* event = (struct inotify_event *)p;
        displayInotifyEvent(event);
        p += sizeof(struct inotify_event) + event->len;
    }

}

int main(int argc, char** argv){
    int nfds = 1;
    int num_open_fds;
    struct pollfd *pfds = calloc(nfds, sizeof(struct pollfd));
    if(pfds == NULL){
        perror("malloc");
        exit(1);
    }

    int inotifyFd = inotify_init();
    if (inotifyFd == -1){
        printf("error while initializing inotify");
        perror("inotify");
    }
    inotify_add_watch(inotifyFd, "./", IN_CREATE | IN_ACCESS | IN_MODIFY );

    pfds[0].fd=inotifyFd;
    pfds[0].events= POLLIN;

    num_open_fds=2;

    while(num_open_fds>0){
        poll(pfds,nfds,-1);

        for (int i = 0; i<nfds; i++){
            if (pfds[i].revents != 0) {
                printf("  fd=%d; events: %s%s%s\n", pfds[i].fd,
                        (pfds[i].revents & POLLIN)  ? "POLLIN "  : "",
                        (pfds[i].revents & POLLHUP) ? "POLLHUP " : "",
                        (pfds[i].revents & POLLERR) ? "POLLERR " : "");

                if (pfds[i].revents & POLLIN) {
                    handle_inotify_event(pfds[i].fd);
                } else {                /* POLLERR | POLLHUP */
                    printf("    closing fd %d\n", pfds[i].fd);
                    if (close(pfds[i].fd) == -1)
                        perror("close");
                        exit(3);
                    num_open_fds--;
                }
            }
        }
    }
    return 0;
}