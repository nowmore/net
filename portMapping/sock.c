//
// Created by x on 8/4/19.
//
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <arpa/inet.h>

#define DEBUG
#include "inc/common.h"
#include "inc/sock.h"


int socket_of_server() {
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0) {
        return ERR_SYSTEM_ERROR;
    }
    socket_set_common_opt(sock);

    //避免对已close的tcp链接写入产生的SIGPIPE导致程序退出
    signal(SIGPIPE, SIG_IGN);
    return sock;
}

int socket_set_timeout(int fd, int flag, time_t timeout) {
    if (timeout < 0 ) {
        return ERR_INVALID_PARAMS;
    }

    struct timeval tv = {
            timeout,
            0
    };

    return setsockopt(fd, SOL_SOCKET, flag == 1 ? SO_RCVTIMEO : SO_SNDTIMEO, &tv, sizeof(tv));
}

int socket_set_common_opt(int fd) {
    int flag = 1;
    //保持连接
    if(-1 == setsockopt(fd,SOL_SOCKET,SO_KEEPALIVE,(char *)&flag, sizeof(flag)))
        return -1;

    flag=1;
    //允许重用本地地址和端口
    if(-1 == setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,(char *)&flag, sizeof(flag)))
        return -1;

    //延迟关闭设置
    struct linger tcp_linger = {
            1,
            2
    };

    return setsockopt(fd, SOL_SOCKET, SO_LINGER, &tcp_linger, sizeof(tcp_linger));
}

int socket_serve(uint16_t port) {

    int sock = socket_of_server();
    if(sock < 0) {
        return ERR_SYSTEM_ERROR;
    }

    struct sockaddr_in sin;
    bzero(&sin, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);

    if(bind(sock, (struct sockaddr*)&sin, sizeof(sin)) < 0 ){
        return ERR_SYSTEM_ERROR;
    }

    if(listen(sock, 5) < 0) {
        close(sock);
        return ERR_SYSTEM_ERROR;
    }

    return sock;
}


int socket_connect(const char *addr, uint16_t port, time_t timeout) {
    if(NULL == addr || timeout < 0) {
        return ERR_INVALID_PARAMS;
    }

    struct sockaddr_in sin;
    bzero(&sin, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    //这里需要判断一下ip是否有效
    if(0 == inet_aton(addr, &sin.sin_addr)){
        printf("invalid ip %s\n", addr);
        return ERR_INVALID_PARAMS;
    }

    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0) {
        return ERR_SYSTEM_ERROR;
    }

    int ret  = socket_set_timeout(sock, 0, timeout);
    if(ret < 0) {
        close(sock);
        return ret;
    }

    ret = connect(sock, (struct sockaddr*)&sin, sizeof(sin));
    if(ret < 0) {
        close(sock);
        return ERR_SYSTEM_ERROR;
    }

    return sock;
}

int serve_accept(int sock, char * client, uint16_t *port) {
    if(sock <= 0) {
        return ERR_INVALID_PARAMS;
    }

    struct sockaddr_in sin;
    unsigned int len = sizeof(sin);
    bzero(&sin, len);

    int cli = accept(sock, (
    struct sockaddr *)&sin, &len);
    if(cli < 0) {
        return ERR_SYSTEM_ERROR;
    }
    if(NULL != client) {
        strcpy(client, inet_ntoa(sin.sin_addr));
    }
    if(NULL != port) {
        *port = sin.sin_port;
    }
    return cli;
}