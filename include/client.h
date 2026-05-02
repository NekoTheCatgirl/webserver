#ifndef WEBSERVER_CLIENT_H
#define WEBSERVER_CLIENT_H

#include <netinet/in.h>

typedef struct client_info_t {
    int fd;
    struct sockaddr_in addr;
} client_info_t;

void client_worker(void *arg);

#endif
