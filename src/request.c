#include <errno.h>
#include <request.h>
#include <stdlib.h>
#include <string.h>

static http_method_t http_method_from_string(const char *s, size_t len) {
    switch (len) {
        case 3:
            if (memcmp(s, "GET", 3) == 0) return HTTP_METHOD_GET;
            if (memcmp(s, "PUT", 3) == 0) return HTTP_METHOD_PUT;
            break;
        case 4:
            if (memcmp(s, "POST", 4) == 0) return HTTP_METHOD_POST;
            if (memcmp(s, "HEAD", 4) == 0) return HTTP_METHOD_HEAD;
            break;
        case 5:
            if (memcmp(s, "PATCH", 5) == 0) return HTTP_METHOD_PATCH;
            if (memcmp(s, "TRACE", 5) == 0) return HTTP_METHOD_TRACE;
            break;
        case 6:
            if (memcmp(s, "DELETE", 6) == 0) return HTTP_METHOD_DELETE;
            break;
        case 7:
            if (memcmp(s, "OPTIONS", 7) == 0) return HTTP_METHOD_OPTIONS;
            if (memcmp(s, "CONNECT", 7) == 0) return HTTP_METHOD_CONNECT;
            break;
        default:
            break;
    }
    return HTTP_METHOD_UNKNOWN;
}

const char *method_to_str(const http_method_t m) {
    switch (m) {
        case HTTP_METHOD_GET:       return "GET";
        case HTTP_METHOD_POST:      return "POST";
        case HTTP_METHOD_PUT:       return "PUT";
        case HTTP_METHOD_DELETE:    return "DELETE";
        case HTTP_METHOD_PATCH:     return "PATCH";
        case HTTP_METHOD_HEAD:      return "HEAD";
        case HTTP_METHOD_OPTIONS:   return "OPTIONS";
        case HTTP_METHOD_TRACE:     return "TRACE";
        case HTTP_METHOD_CONNECT:   return "CONNECT";
        default:                    return "UNKNOWN";
    }
}

static int push_header(http_request_t *req, const char *name, size_t name_len, const char *val, size_t val_len) {
    if (req->header_count == req->header_cap) {
        const size_t new_cap = req->header_cap == 0 ? 8 : req->header_cap * 2;
        if (new_cap > HTTP_MAX_HEADERS) return -1;
        http_header_t *h = realloc(req->headers, new_cap * sizeof(http_header_t));
        if (!h) return -1;
        req->headers = h;
        req->header_cap = new_cap;
    }

    http_header_t *h = &req->headers[req->header_count++];
    h->name = strndup(name, name_len);
    h->value = strndup(val, val_len);
    if (!h->name || !h->value) return -1;
    h->name_len = name_len;
    h->value_len = val_len;
    return 0;
}

int http_request_parse(http_request_t *req, const char *raw, size_t len) {
    memset(req, 0, sizeof(*req));

    const char *cur = raw;
    const char *end = raw + len;

    const char *method_end = memchr(cur, ' ', end - cur);
    if (!method_end) goto err;
    req->method = http_method_from_string(cur, method_end - cur);
    cur = method_end + 1;

    const char *uri_end = memchr(cur, ' ', end - cur);
    if (!uri_end) goto err;
    size_t uri_len = uri_end - cur;
    if (uri_len > HTTP_MAX_URI_LEN) goto err;

    req->uri = strndup(cur, uri_len);
    if (!req->uri) goto err;

    const char *q = memchr(cur, '?', uri_len);
    if (q) {
        req->path = strndup(cur, q - cur);
        req->query = strndup(q + 1, uri_end - (q + 1));
        if (!req->path || !req->query) goto err;
    } else {
        req->path = strndup(cur, uri_len);
        if (!req->path) goto err;
    }
    cur = uri_end + 1;

    const char *ver_end = memmem(cur, end - cur, "\r\n", 2);
    if (!ver_end) goto err;
    req->version = strndup(cur, ver_end - cur);
    if (!req->version) goto err;
    cur = ver_end + 2;

    while (cur < end) {
        if (cur[0] == '\r' && cur[1] == '\n') {
            cur += 2;
            break;
        }

        const char *line_end = memmem(cur, end - cur, "\r\n", 2);
        if (!line_end) goto err;

        const char *colon = memchr(cur, ':', line_end - cur);
        if (!colon) goto err;

        const char *name = cur;
        const size_t name_len = colon - cur;

        const char *val = colon + 1;
        while (val < line_end && *val == ' ') val++;
        const size_t val_len = line_end - val;

        if (push_header(req, name, name_len, val, val_len) != 0) goto err;

        cur = line_end + 2;
    }

    const char *cl = http_request_header(req, "Content-Length");
    if (cl) {
        char *endptr;
        errno = 0;
        const long body_len_l = strtol(cl, &endptr, 10);
        if (errno != 0 || endptr == cl || body_len_l < 0) goto err;

        const size_t body_len = (size_t)body_len_l;
        if (body_len > HTTP_MAX_BODY_LEN) goto err;
        if (cur + body_len > end) goto err;

        req->body = strndup(cur, body_len);
        if (!req->body) goto err;
        req->body_len = body_len;
    }

    return 0;

err:
    http_request_free(req);
    return -1;
}

const char *http_request_header(const http_request_t *req, const char *name) {
    for (int i = 0; i < req->header_count; i++) {
        if (strcasecmp(req->headers[i].name, name) == 0)
            return req->headers[i].value;
    }
    return nullptr;
}

void http_request_free(http_request_t *req) {
    for (int i = 0; i < req->header_count; i++) {
        free(req->headers[i].name);
        free(req->headers[i].value);
    }
    free(req->headers);
    free(req->uri);
    free(req->path);
    free(req->query);
    free(req->version);
    free(req->body);
    memset(req, 0, sizeof(*req));
}
