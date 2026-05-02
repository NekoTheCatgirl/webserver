#include <client.h>
#include <dispatcher.h>
#include <dynstr.h>
#include <errno.h>
#include <request.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "status.h"

// ReSharper disable once CppParameterMayBeConstPtrOrRef
void client_worker(void *arg) {
    if (arg == nullptr) return;

    client_info_t *info = arg;
    const int client_fd = info->fd;

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &info->addr.sin_addr, client_ip, sizeof(client_ip));
    uint16_t client_port = ntohs(info->addr.sin_port);

    free(info);

    dynstr_t *request_string = dynstr_create();
    char buffer[4096];
    char *end_of_headers;

    while (true) {
        const ssize_t bytes_received = recv(client_fd, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) {
            dynstr_destroy(request_string);
            close(client_fd);
            return;
        }

        dynstr_append(request_string, buffer, bytes_received);

        const char *data = dynstr_get(request_string);
        const size_t current_len = dynstr_len(request_string);

        end_of_headers = (char*)memmem(data, current_len, "\r\n\r\n", 4);
        if (end_of_headers) {
            break;
        }
        
        if (current_len > 16384) {
            dynstr_destroy(request_string);
            close(client_fd);
            return;
        }
    }

    const size_t total_header_size = (end_of_headers + 4) - dynstr_get(request_string);
    size_t body_received = dynstr_len(request_string) - total_header_size;

    const char saved_char = end_of_headers[4];
    end_of_headers[4] = '\0';
    
    long content_length = 0;
    const char *cl = strcasestr(dynstr_get(request_string), "Content-Length:");
    if (cl) {
        cl += 15;
        while (*cl == ' ') cl++;
        char *endptr;
        errno = 0;
        content_length = strtol(cl, &endptr, 10);
        if (errno != 0 || endptr == cl) {
            content_length = 0;
        }
    }
    end_of_headers[4] = saved_char;

    while (body_received < (size_t)content_length) {
        const ssize_t n = recv(client_fd, buffer, sizeof(buffer), 0);
        if (n <= 0) break;
        dynstr_append(request_string, buffer, n);
        body_received += n;
    }

    const char *raw_request = dynstr_get(request_string);
    const size_t raw_request_len = dynstr_len(request_string);

    request_t req;
    response_t *res;
    if (http_request_parse(&req, raw_request, raw_request_len) == 0) {
        printf("%s:%u -> %s\n", client_ip, client_port, req.path);
        res = dispatch(req.path, &req);
        http_request_free(&req);
    } else {
        printf("%s:%u -> Bad Request\n", client_ip, client_port);
        res = create_response("HTTP/1.1", STATUS_BAD_REQUEST, nullptr);
        set_content(res, "Bad Request");
    }

    dynstr_destroy(request_string);

    char *response_str = response_to_string(res);
    destroy_response(res);

    if (response_str) {
        send(client_fd, response_str, strlen(response_str), 0);
        free(response_str);
    }

    close(client_fd);
}
