#ifndef WEBSERVER_REQUEST_H
#define WEBSERVER_REQUEST_H

#include <stddef.h>
#include <http.h>

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
typedef enum method_t {
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
} method_t;

typedef struct request_t {
    method_t method;
    char *uri;
    char *path;
    char *query;
    char *version;
    char *body;
    size_t body_len;
    http_header_t *headers;
    size_t header_count;
    size_t header_cap;
} request_t;

const char *method_to_str(method_t m);
int http_request_parse(request_t *req, const char *raw, size_t len);
const char *http_request_header(const request_t *req, const char *name);
void http_request_free(request_t *req);

#endif
