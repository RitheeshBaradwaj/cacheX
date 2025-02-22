#include "server.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

// Simple hash table implementation
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

void handle_client_request(int client_fd) {
    char buffer[1024];
    ssize_t bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        close(client_fd);
        return;
    }

    buffer[bytes_received] = '\0';  // Null-terminate the received data
    fprintf("Client %d: %s\n", client_fd, buffer);

    char command[10], key[256], value[256];
    if (sscanf(buffer, "%9s %255s %255[^\n]", command, key, value) == 3 &&
        strcmp(command, "SET") == 0) {
        store_set_command(key, value);
        send(client_fd, "OK\n", 3, 0);
    } else if (sscanf(buffer, "%9s %255s", command, key) == 2 && strcmp(command, "GET") == 0) {
        const char *result = fetch_get_command(key);
        if (result) {
            send(client_fd, result, strlen(result), 0);
            send(client_fd, "\n", 1, 0);
        } else {
            send(client_fd, "NULL\n", 5, 0);
        }
    } else {
        send(client_fd, "ERROR\n", 6, 0);
    }
}

void start_server() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        die(__LINE__, "%s: socket(), errno: %d", __func__, errno);
    }

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
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

    fprintf("Server started on port %d, ready for GET/SET...\n", PORT);

    struct epoll_event event, events[MAX_EVENTS];
    int epoll_fd = epoll_create1(0);
    event.events = EPOLLIN;
    event.data.fd = server_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event);

    while (1) {
        int num_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        for (int i = 0; i < num_events; i++) {
            if (events[i].data.fd == server_fd) {
                // accept
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
                fprintf("New client connected: %d\n", client_fd);
            } else {
                handle_client_request(events[i].data.fd);
            }
        }
    }

    close(server_fd);
}
