#ifndef WEBSERVER_REQUEST_H
#define WEBSERVER_REQUEST_H

#include <stddef.h>

#ifndef HTTP_MAX_HEADERS
#define HTTP_MAX_HEADERS 64
#endif

#ifndef HTTP_MAX_URI_LEN
#define HTTP_MAX_URI_LEN 2048
#endif

#ifndef HTTP_MAX_BODY_LEN
#define HTTP_MAX_BODY_LEN 1048576 // 1MB
#endif

#if defined(__GNUC__) || defined(__clang__)
typedef enum __attribute__((packed)) http_method_t {
#else
typedef enum http_method_t {
#endif
    HTTP_METHOD_UNKNOWN = 0,
    HTTP_METHOD_GET,
    HTTP_METHOD_POST,
    HTTP_METHOD_PUT,
    HTTP_METHOD_DELETE,
    HTTP_METHOD_PATCH,
    HTTP_METHOD_HEAD,
    HTTP_METHOD_OPTIONS,
    HTTP_METHOD_TRACE,
    HTTP_METHOD_CONNECT,
} http_method_t;

typedef struct http_header_t {
    char *name;
    char *value;
    size_t name_len;
    size_t value_len;
} http_header_t;

typedef struct http_request_t {
    http_method_t method;
    char *uri;
    char *path;
    char *query;
    char *version;
    char *body;
    size_t body_len;
    http_header_t *headers;
    size_t header_count;
    size_t header_cap;
} http_request_t;

const char *method_to_str(http_method_t m);
int http_request_parse(http_request_t *req, const char *raw, size_t len);
const char *http_request_header(const http_request_t *req, const char *name);
void http_request_free(http_request_t *req);

#endif
