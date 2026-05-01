#include <client.h>
#include <dynstr.h>
#include <errno.h>
#include <request.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

// ReSharper disable once CppParameterMayBeConstPtrOrRef
void client_worker(void *arg) {
    if (arg == nullptr) return;

    const int client_fd = *(int *)arg;
    free(arg);

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

    size_t total_header_size = (end_of_headers + 4) - dynstr_get(request_string);
    size_t body_received = dynstr_len(request_string) - total_header_size;

    char saved_char = end_of_headers[4];
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
    size_t raw_request_len = dynstr_len(request_string);

    printf("Received request of length %zu\n", raw_request_len);

    http_request_t req;
    if (http_request_parse(&req, raw_request, raw_request_len) == 0) {
        printf("Received request method: %s\n", method_to_str(req.method));
        printf("Received request path: %s\n", req.path);
        if (req.body) {
            printf("Received request body: %s\n", req.body);
        }
        fflush(stdout);
        http_request_free(&req);
    } else {
        printf("Failed to parse request\n");
        fflush(stdout);
    }

    dynstr_destroy(request_string);

    const char *response = "HTTP/1.1 200 OK\r\nContent-Length: 12\r\n\r\nHello World!";
    send(client_fd, response, strlen(response), 0);
    close(client_fd);
}
