#include <response.h>
#include <dynstr.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct [[maybe_unused]] response_t {
    char *version;
    uint16_t status;
    char *status_text;
    char *content;
    size_t content_length;
    http_header_t *headers;
    size_t header_count;
    size_t header_cap;
};

static int status_code_to_str(const uint16_t code, char *buf, const size_t buf_len) {
    const char *text;

    switch (code) {
        // 1xx Informational
        case 100: text = "Continue"; break;
        case 101: text = "Switching Protocols"; break;
        case 102: text = "Processing"; break;
        case 103: text = "Early Hints"; break;

        // 2xx Success
        case 200: text = "OK"; break;
        case 201: text = "Created"; break;
        case 202: text = "Accepted"; break;
        case 203: text = "Non-Authoritative Information"; break;
        case 204: text = "No Content"; break;
        case 205: text = "Reset Content"; break;
        case 206: text = "Partial Content"; break;
        case 207: text = "Multi-Status"; break;
        case 208: text = "Already Reported"; break;
        case 226: text = "IM Used"; break;

        // 3xx Redirection
        case 300: text = "Multiple Choices"; break;
        case 301: text = "Moved Permanently"; break;
        case 302: text = "Found"; break;
        case 303: text = "See Other"; break;
        case 304: text = "Not Modified"; break;
        case 307: text = "Temporary Redirect"; break;
        case 308: text = "Permanent Redirect"; break;

        // 4xx Client Errors
        case 400: text = "Bad Request"; break;
        case 401: text = "Unauthorized"; break;
        case 402: text = "Payment Required"; break;
        case 403: text = "Forbidden"; break;
        case 404: text = "Not Found"; break;
        case 405: text = "Method Not Allowed"; break;
        case 406: text = "Not Acceptable"; break;
        case 407: text = "Proxy Authentication Required"; break;
        case 408: text = "Request Timeout"; break;
        case 409: text = "Conflict"; break;
        case 410: text = "Gone"; break;
        case 411: text = "Length Required"; break;
        case 412: text = "Precondition Failed"; break;
        case 413: text = "Content Too Large"; break;
        case 414: text = "URI Too Long"; break;
        case 415: text = "Unsupported Media Type"; break;
        case 416: text = "Range Not Satisfiable"; break;
        case 417: text = "Expectation Failed"; break;
        case 418: text = "I'm a Teapot"; break;
        case 421: text = "Misdirected Request"; break;
        case 422: text = "Unprocessable Content"; break;
        case 423: text = "Locked"; break;
        case 424: text = "Failed Dependency"; break;
        case 425: text = "Too Early"; break;
        case 426: text = "Upgrade Required"; break;
        case 428: text = "Precondition Required"; break;
        case 429: text = "Too Many Requests"; break;
        case 431: text = "Request Header Fields Too Large"; break;
        case 451: text = "Unavailable For Legal Reasons"; break;

        // 5xx Server Errors
        case 500: text = "Internal Server Error"; break;
        case 501: text = "Not Implemented"; break;
        case 502: text = "Bad Gateway"; break;
        case 503: text = "Service Unavailable"; break;
        case 504: text = "Gateway Timeout"; break;
        case 505: text = "HTTP Version Not Supported"; break;
        case 506: text = "Variant Also Negotiates"; break;
        case 507: text = "Insufficient Storage"; break;
        case 508: text = "Loop Detected"; break;
        case 510: text = "Not Extended"; break;
        case 511: text = "Network Authentication Required"; break;

        default: return -1;
    }

    snprintf(buf, buf_len, "%s", text);
    return 0;
}

response_t * create_response(const char *version, const uint16_t status, const char *status_text) {
    response_t *r = calloc(1, sizeof(response_t));
    if (!r) return nullptr;

    r->version = strdup(version);
    r->status = status;
    char status_buf[64];
    const char *status_text_final;

    if (status_code_to_str(status, status_buf, sizeof(status_buf)) == 0)
        status_text_final = status_buf;
    else
        status_text_final = status_text;

    r->status_text = strdup(status_text_final);
    r->header_cap = 8;
    r->headers = malloc(sizeof(http_header_t) * r->header_cap);

    if (!r->version || !r->status_text || !r->headers) {
        destroy_response(r);
        return nullptr;
    }

    return r;
}

void set_content(response_t *response, const char *content) {
    if (!response) return;

    // Free existing content if it already exists.
    if (response->content != nullptr) {
        free(response->content);

        response->content = nullptr;
        response->content_length = 0;
    }

    // If no content provided, leave the content and len at null/0
    if (!content) return;

    response->content = strdup(content);
    if (!response->content) return;
    response->content_length = strlen(response->content);
}

void add_or_modify_header(response_t *response, http_header_t *header) {
    if (!response || !header) return;

    static constexpr size_t cl_header_len = sizeof("Content-Length") - 1;

    // Content length is handled automatically, don't allow setting it by the user
    if (header->name_len == cl_header_len && strncasecmp(header->name, "Content-Length", 14) == 0) return;

    for (size_t i = 0; i < response->header_count; i++) {
        if (response->headers[i].name_len == header->name_len &&
            strncasecmp(response->headers[i].name, header->name, header->name_len) == 0) {
            free(response->headers[i].value);
            response->headers[i].value = strndup(header->value, header->value_len);
            response->headers[i].value_len = header->value_len;
            return;
        }
    }

    if (response->header_count == response->header_cap) {
        const size_t new_cap = response->header_cap * 2;
        http_header_t *grown = realloc(response->headers, sizeof(http_header_t) * new_cap);
        if (!grown) return;
        response->headers = grown;
        response->header_cap = new_cap;
    }

    http_header_t *slot = &response->headers[response->header_count++];
    slot->name = strndup(header->name, header->name_len);
    slot->name_len = header->name_len;
    slot->value = strndup(header->value, header->value_len);
    slot->value_len = header->value_len;
}

void add_or_modify_header_simple(response_t *response, const char *header_name, const char *header_value) {
    if (!response || !header_name || !header_value) return;

    http_header_t header = {
        .name = (char *)header_name,
        .name_len = strlen(header_name),
        .value = (char *)header_value,
        .value_len = strlen(header_value)
    };

    add_or_modify_header(response, &header);
}

char *response_to_string(response_t *response) {
    if (!response) return nullptr;

    dynstr_t *ds = dynstr_create();
    if (!ds) return nullptr;

    char status_line[64];
    const int sl_len = snprintf(status_line, sizeof(status_line),
        "%s %u %s\r\n",
        response->version, response->status, response->status_text
    );

    dynstr_append(ds, status_line, sl_len);

    /* Content-Length header */
    char cl_buf[32];
    const int cl_len = snprintf(cl_buf, sizeof(cl_buf), "Content-Length: %zu\r\n", response->content_length);
    dynstr_append(ds, cl_buf, cl_len);

    /* User-supplied headers */
    for (size_t i = 0; i < response->header_count; i++) {
        dynstr_append(ds, response->headers[i].name, response->headers[i].name_len);
        dynstr_append(ds, ": ", 2);
        dynstr_append(ds, response->headers[i].value, response->headers[i].value_len);
        dynstr_append(ds, "\r\n", 2);
    }

    /* Blank line separating headers from body */
    dynstr_append(ds, "\r\n", 2);

    if (response->content && response->content_length > 0)
        dynstr_append(ds, response->content, response->content_length);

    /* Null terminate so we can return a plain C string */
    dynstr_append(ds, "\0", 1);

    char *out = strdup(dynstr_get(ds));

    dynstr_destroy(ds);
    return out;
}

void destroy_response(response_t *response) {
    if (!response) return;

    free(response->version);
    free(response->status_text);
    free(response->content);

    for (size_t i = 0; i < response->header_count; i++) {
        free(response->headers[i].name);
        free(response->headers[i].value);
    }
    free(response->headers);
    free(response);
}
