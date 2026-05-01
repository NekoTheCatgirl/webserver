#include <server.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdio.h>

struct server_t {
    int fd;
    tpool_t *pool;
    bool stop;
};

// ReSharper disable once CppParameterMayBeConstPtrOrRef
static void server_worker(void *arg) {
    const server_t *server = arg;

    while (1) {
        if (server->stop) break;

        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_fd = accept(server->fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) continue;

        int *client_fd_arg = malloc(sizeof(*client_fd_arg));
        if (!client_fd_arg) {
            close(client_fd);
            continue;
        }

        *client_fd_arg = client_fd;

        if (!tpool_add_work(server->pool, client_worker, client_fd_arg)) {
            close(client_fd);
            free(client_fd_arg);
        }
    }
}

server_t *server_create(const uint64_t addr, const uint16_t port, tpool_t *pool, int backlog) {
    if (pool == nullptr) {
        errno = EINVAL;
        return nullptr;
    }

    const int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    constexpr int reuse = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    struct sockaddr_in sock_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr.s_addr = addr
    };

    if (bind(server_fd, (struct sockaddr *)&sock_addr, sizeof(sock_addr)) < 0) {
        close(server_fd);
        return nullptr;
    }

    if (listen(server_fd, backlog) < 0) {
        close(server_fd);
        return nullptr;
    }

    // Only malloc if everything is valid and nothing fails.
    server_t *server = malloc(sizeof(server_t));
    if (server == nullptr) {
        close(server_fd);
        return nullptr;
    }

    server->fd = server_fd;
    server->pool = pool;
    server->stop = false;

    if (!tpool_add_work(pool, server_worker, server)) {
        const int saved_errno = errno == 0 ? EAGAIN : errno;
        close(server_fd);
        free(server);
        errno = saved_errno;
        return nullptr;
    }

    return server;
}

void server_destroy(server_t *server) {
    if (server == nullptr) return;

    server->stop = true;
    // Important, close the server socket before freeing
    shutdown(server->fd, SHUT_RDWR);
    close(server->fd);
    tpool_wait(server->pool);
    // We don't actually truly own the pointer to the pool, so we do not free it
    free(server);
    printf("Server resources cleaned up successfully.\n");
}
