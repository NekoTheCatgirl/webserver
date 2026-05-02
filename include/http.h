#ifndef WEBSERVER_HTTP_H
#define WEBSERVER_HTTP_H

#include <stddef.h>

typedef struct header_t {
    char *name;
    char *value;
    size_t name_len;
    size_t value_len;
} http_header_t;

#endif
