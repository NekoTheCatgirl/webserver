#ifndef WEBSERVER_DISPATCHER_H
#define WEBSERVER_DISPATCHER_H

#include "response.h"
#include "request.h"
#include <string.h>

response_t *handle_index(const request_t *req);

#define ROUTES \
    X("/", handle_index) \

static inline response_t *dispatch(const char *path, const request_t *req) {
#define X(p, handler) if (strcmp(path, p) == 0) return handler(req);
    ROUTES
#undef X
    // No route matched — 404
    response_t *res = create_response("HTTP/1.1", 404, nullptr);
    set_content(res, "Not Found");
    return res;
}

#endif
