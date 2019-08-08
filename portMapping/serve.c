//
// Created by x on 8/6/19.
//
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <time.h>
#include <signal.h>

#include "inc/common.h"
#include "inc/sock.h"

int sock = 0;

handler task = {
        .count = 0,
        .h = LIST_HEAD_INIT(task.h)
};

handler idle = {
        .count = 0,
        .h = LIST_HEAD_INIT(idle.h)
};

pair pairs[MAX_CLI];

void release(int status) {
    if(sock > 0)
        close(sock);

    pair *p = NULL;
    list_for_each_entry(p, &(task.h), ptr){
        close(p->sock[0]);
        close(p->sock[1]);
    }

    exit(status);
}

void sig_handle(int sig) {
    release(1);
}


void task_move(pair *p, handler * from, handler *to, int closed) {
    if(closed) {
        close(p->sock[0]);
        close(p->sock[1]);
    }
    list_del(&p->ptr);
    list_add(&p->ptr, &(to->h));
    --from->count;
    ++to->count;
}
void init() {
    for (int i = 0; i < MAX_CLI; ++i) {
        INIT_LIST_HEAD(&pairs[i].ptr);
        list_add(&(pairs[i].ptr), &(idle.h));
        idle.count++;
    }
}

void usage() {
    fprintf(stderr,
            "usage: portMapping [-H|--host] [-p|--port] "
            "[-h|--help]\n");
    exit(1);
}

void get_options(int argc, char **argv, char *addr, uint16_t *local, uint16_t *remote) {
    if(3 > argc || NULL == argv
       || NULL == argv || NULL == remote || NULL == local) {
        usage();
    }

    int opt;
    const char *short_options = "l:H:p:h" ;
    int option_index = 0;
    static struct option long_options[] =
            {
                    {"listen",  required_argument,  NULL,'l'},
                    {"host",    required_argument,  NULL,'H'},
                    {"port",    required_argument,  NULL,'p'},
                    {"help",    no_argument,        NULL,'h'},
            };
    while((opt = getopt_long_only(argc, argv, short_options, long_options, &option_index)) != -1)
    {
        switch (opt) {
            case 'l':
                *local = atoi(optarg);
                break;
            case 'p':
                *remote = atoi(optarg);
                break;
            case 'H':
                strcpy(addr, optarg);
                break;
            default:
                usage();
        }
    }
}

void serve_loop(char *addr, uint16_t port) {
    int max_fd = 0;
    char buf[MAX_BUF] = {0};
    fd_set read_set;
    pair *p = NULL, *q = NULL;
    struct  timeval tv = {
            0,
            50000
    };

    while(1) {
        p = NULL;
        q = NULL;
        max_fd = sock;
        tv.tv_usec = 50000;
        FD_ZERO(&read_set);
        FD_SET(sock, &read_set);
        list_for_each_entry(p, &(task.h), ptr){
            FD_SET(p->sock[0], &read_set);
            FD_SET(p->sock[1], &read_set);
            max_fd = p->sock[0] > max_fd ? p->sock[0] : max_fd;
            max_fd = p->sock[1] > max_fd ? p->sock[1] : max_fd;
        }
        if(select(max_fd + 1, &read_set, NULL, NULL, &tv) < 0) {
            continue;
        }
        if(FD_ISSET(sock, &read_set)) {
            if(task.count >= MAX_CLI) {
                printf("too many connections!\n");
            } else {
                char cli[31];
                uint16_t cli_port;
                p = list_entry((idle.h).next, typeof(*p), ptr);
                p->sock[0] = serve_accept(sock, cli, &cli_port);
                if(p->sock[0] <= 0){
                    perror("accept error!");
                    continue;
                }
                p->sock[1] = socket_connect(addr, port, 1);
                if(p->sock[1] <= 0) {
                    printf("connect to %s:%d failed!\n", addr, port);
                    close(p->sock[0]);
                    continue;
                }
                p->since = time(NULL);
                task_move(p, &idle, &task, 0);
                printf("establish a connection between %s:%d and %s:%d\n",
                        cli, cli_port, addr, port);
            }
        }

        list_for_each_entry_safe(p, q, &(task.h), ptr){
            int from, to, bytes;
            if(FD_ISSET(p->sock[0], &read_set)) {
                from = p->sock[0];
                to = p->sock[1];
            } else if(FD_ISSET(p->sock[1], &read_set)){
                from = p->sock[1];
                to = p->sock[0];
            } else {
                if(time(NULL) - p->since > MAX_IDLE_TIME) {
                    task_move(p, &task, &idle, 1);
                }
                continue;
            }
            if((bytes = read(from, buf, sizeof(buf))) <= 0
            || write(to, buf, bytes) <= 0) {
                task_move(p, &task, &idle, 1);
            } else {
                p->since = time(NULL);
            }
        }

    }

}

void serve(int argc, char **argv) {
    uint16_t local, remote;
    char addr[31] = {0};

    get_options(argc, argv, addr, &local, &remote);

    errno = 0;
    sock = socket_serve(local);
    if(sock == -1) {
        perror("create server failed!");
        exit(-1);
    }
    init();
    printf("server starting at 0.0.0.0:%d\n", local);
    signal(SIGINT, sig_handle);

    serve_loop(addr, remote);
}