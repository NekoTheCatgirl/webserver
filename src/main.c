#include <errno.h>
#include <getopt.h>
#include <pthread.h>
#include <server.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/signal.h>

#ifndef NUM_THREADS
#define NUM_THREADS 4
#endif

volatile sig_atomic_t running = 1;

static void catch_function(const int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        printf("Shutting down, please be patient and let all resources be cleaned up!\n");
        running = 0;
    }
}

void print_help(const char *program_name) {
    printf("Usage: %s --addr <ip_addr> --port <port_num>\n\n", program_name);
    printf("Arguments:\n");
    printf("  --addr <ip_addr>: IP address to bind to (default 127.0.0.1)\n");
    printf("  --port <port_num>: Port number to listen on (default 8080)\n");
    printf("  --help: Display this help message\n");
}

int main(const int argc, char *argv[]) {
    if (signal(SIGINT, catch_function) == SIG_ERR || signal(SIGTERM, catch_function) == SIG_ERR) {
        fprintf(stderr, "Failed to set signal handler: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Initialize as localhost if args specify --addr with a 4-byte sequence separated by dots, overwrite this
    uint8_t addr[4] = {127, 0, 0, 1};
    // Default port can be overwritten by --port argument
    uint16_t port = 8080;

    static struct option long_options[] = {
        {"addr", optional_argument, nullptr, 'a'},
        {"port", optional_argument, nullptr, 'p'},
        {"help", no_argument, nullptr, 'h'},
        {nullptr, 0, nullptr, 0}
    };

    int opt; // Read cli arguments if any.
    while ((opt = getopt_long(argc, argv, "a::p::h", long_options, nullptr)) != -1) {
        switch (opt) {
            case 'a':
                if (optarg != nullptr) {
                    if (inet_pton(AF_INET, optarg, addr) <= 0) {
                        fprintf(stderr, "Invalid IP address: %s\n", optarg);
                        exit(EXIT_FAILURE);
                    }
                }
                break;
            case 'p':
                if (optarg != nullptr) {
                    char *endptr;
                    errno = 0;
                    const long val = strtol(optarg, &endptr, 10);
                    if (endptr == optarg || *endptr != '\0' || errno == ERANGE) {
                        fprintf(stderr, "Invalid port number: %s\n", optarg);
                        exit(EXIT_FAILURE);
                    }
                    if (val < 1 || val > 65535) {
                        fprintf(stderr, "Port number must be between 1 and 65535\n");
                        exit(EXIT_FAILURE);
                    }
                    port = val;
                }
                break;
            case 'h':
                print_help(argv[0]);
                exit(EXIT_SUCCESS);
            case '?':
                print_help(argv[0]);
                exit(EXIT_FAILURE);
            default:
                break;
        }
    }

    errno = 0;
    tpool_t *pool = tpool_create(NUM_THREADS);
    if (pool == nullptr || errno != 0) {
        fprintf(stderr, "Failed to create thread pool: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    errno = 0;
    server_t *server = server_create(INADDR_ANY, port, pool, 128);
    if (server == nullptr || errno != 0) {
        fprintf(stderr, "Failed to create server: %s\n", strerror(errno));
        tpool_destroy(pool);
        exit(EXIT_FAILURE);
    }

    while (1) {
        if (running == 0) {
            break;
        }
    }

    server_destroy(server);
    tpool_destroy(pool);
    exit(EXIT_SUCCESS);
}
