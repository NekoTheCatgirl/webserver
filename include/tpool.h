#ifndef WEBSERVER_TPOOL_H
#define WEBSERVER_TPOOL_H

#include <stddef.h>

typedef struct tpool_t tpool_t;

typedef void (*thread_func_t)(void *arg);

tpool_t *tpool_create(size_t num);
void tpool_destroy(tpool_t *pool);

bool tpool_add_work(tpool_t *pool, thread_func_t func, void *arg);
void tpool_wait(tpool_t *pool);

#endif
