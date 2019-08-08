//
// Created by x on 8/4/19.
//

#ifndef TCP_SOCK_H
#define TCP_SOCK_H

#include <stdint.h>
int socket_connect(const char *addr, uint16_t port, time_t timeout);
int socket_set_common_opt(int fd);
int socket_set_timeout(int fd, int flag, time_t timeout);
int socket_of_server();
int socket_serve(uint16_t port);
int serve_accept(int sock, char * client, uint16_t *port);
#endif //TCP_SOCK_H
