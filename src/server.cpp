#include "server.hpp"

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

#include "cacheX_protocol.hpp"

typedef struct KeyValue {
    char *key;
    char *value;
    struct KeyValue *next;
} KeyValue;

KeyValue *hash_table[HASH_TABLE_SIZE] = {NULL};

// Hash function (djb2 algorithm)
unsigned long hash_function(const char *str) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    return hash % HASH_TABLE_SIZE;
}

void store_set_command(const char *key, const char *value) {
    unsigned long index = hash_function(key);

    KeyValue *new_pair = (KeyValue *)malloc(sizeof(KeyValue));
    new_pair->key = strdup(key);
    new_pair->value = strdup(value);
    new_pair->next = hash_table[index];

    hash_table[index] = new_pair;
}

const char *fetch_get_command(const char *key) {
    unsigned long index = hash_function(key);
    KeyValue *entry = hash_table[index];

    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            return entry->value;
        }
        entry = entry->next;
    }
    return NULL;
}

static void die(int line_number, const char *format, ...) {
    va_list vargs;
    va_start(vargs, format);
    fprintf(stderr, "%d: ", line_number);
    vfprintf(stderr, format, vargs);
    fprintf(stderr, ".\n");
    va_end(vargs);
    exit(1);
}

int set_nonblocking(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);                // get the flags
    int rv = fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);  // modify and set the flags
    if (rv) {
        die(__LINE__, "%s: fcntl(), errno: %d", __func__, errno);
    }
    return rv;
}

static int32_t handle_client_request(int client_fd) {
    char request_payload[MAX_PAYLOAD_SIZE];

    // Read structured request (response from client is like getting request from client)
    int res = receive_response(client_fd, request_payload, sizeof(request_payload));
    if (res < 0) {
        fprintf(stderr, "[INFO] Client %d disconnected.\n", client_fd);
        close(client_fd);
        return -1;
    }

    fprintf(stderr, "[DEBUG] Client %d request: %s\n", client_fd, request_payload);
    char command[10], key[256], value[256];
    if (sscanf(request_payload, "%9s %255s %255[^\n]", command, key, value) == 3 &&
        strcmp(command, "SET") == 0) {
        store_set_command(key, value);

        // like send response to client
        return send_request(client_fd, "OK");
    } else if (sscanf(request_payload, "%9s %255s", command, key) == 2 &&
               strcmp(command, "GET") == 0) {
        const char *result = fetch_get_command(key);
        fprintf(stderr, "[DEBUG] GET key: %s, Value: %s\n", key, result ? result : "NULL");
        return send_request(client_fd, result ? result : "NULL");
    } else {
        fprintf(stderr, "[ERROR] Invalid command from client %d\n", client_fd);
        return send_request(client_fd, "ERROR");
    }
}

void start_server() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        die(__LINE__, "%s: socket(), errno: %d", __func__, errno);
    }

    int level = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &(level), sizeof(level));
    set_nonblocking(server_fd);

    // bind
    struct sockaddr_in server_addr = {};
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;  // OR htonl(0)
    server_addr.sin_port = htons(PORT);

    int rv = bind(server_fd, (const struct sockaddr *)&server_addr, sizeof(server_addr));
    if (rv) {
        die(__LINE__, "%s: bind(), errno: %d", __func__, errno);
    }

    // listen
    rv = listen(server_fd, BACKLOG);
    if (rv) {
        die(__LINE__, "%s: listen(), errno: %d", __func__, errno);
    }

    printf("Server started on port %d, ready for GET/SET...\n", PORT);

    struct epoll_event event, events[MAX_EVENTS];
    int epoll_fd = epoll_create1(0);
    event.events = EPOLLIN;
    event.data.fd = server_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event);

    while (true) {
        int num_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        for (int i = 0; i < num_events; i++) {
            if (events[i].data.fd == server_fd) {
                // Accept new client
                struct sockaddr_in client_addr = {};
                socklen_t client_len = sizeof(client_addr);
                int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
                if (client_fd < 0) {
                    continue;  // error
                }
                set_nonblocking(client_fd);
                event.events = EPOLLIN | EPOLLET;
                event.data.fd = client_fd;
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event);
                printf("New client connected: %d\n", client_fd);
            } else {
                // Handle request for existing client
                int client_fd = events[i].data.fd;
                int err = handle_client_request(client_fd);
                if (err < 0) {
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
                } else {
                    // Re-enable EPOLLIN so more requests can be processed
                    event.events = EPOLLIN | EPOLLET;
                    event.data.fd = client_fd;
                    epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_fd, &event);
                }
            }
        }
    }

    close(server_fd);
}
