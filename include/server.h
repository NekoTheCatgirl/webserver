#ifndef WEBSERVER_SERVER_H
#define WEBSERVER_SERVER_H

#include <stdint.h>
#include <tpool.h>

typedef struct server_t server_t;

server_t *server_create(uint64_t addr, uint16_t port, tpool_t *pool, int backlog);

void server_destroy(server_t *server);

#endif
