//
// Created by x on 8/4/19.
//

#ifndef TCP_COMMON_H
#define TCP_COMMON_H

#define ERR_SYSTEM_ERROR    -1  //系统错误
#define ERR_INVALID_PARAMS  -2  //无效参数

#include "list.h"
#define MAX_CLI 1024
#define MAX_BUF 8192
#define MAX_IDLE_TIME 1800

typedef struct __handler {
    int count;
    struct list_head h;
}handler;

typedef struct __pair {
    int sock[2];
    long int since;
    struct list_head ptr;
}pair;

void serve(int argc, char **argv);
#endif //TCP_COMMON_H
