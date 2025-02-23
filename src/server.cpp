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

#include <string>

#include "cacheX_protocol.hpp"

static std::vector<Conn *> fd2conn;

typedef struct KeyValue {
    char *key;
    char *value;
    struct KeyValue *next;
} KeyValue;

KeyValue *hash_table[HASH_TABLE_SIZE] = {nullptr};

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
    if (!new_pair) {
        fprintf(stderr, "[ERROR] Memory allocation failed for KeyValue\n");
        return;
    }

    new_pair->key = strdup(key);
    new_pair->value = strdup(value);
    if (!new_pair->key || !new_pair->value) {
        fprintf(stderr, "[ERROR] Memory allocation failed for key/value\n");
        free(new_pair);
        return;
    }

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
    return nullptr;
}

static void msg(int line_number, const char *format, ...) {
    va_list vargs;
    va_start(vargs, format);
    fprintf(stderr, "%d: ", line_number);
    vfprintf(stderr, format, vargs);
    fprintf(stderr, ".\n");
    va_end(vargs);
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

static int set_nonblocking(int sockfd) {
    errno = 0;
    int flags = fcntl(sockfd, F_GETFL, 0);  // get the flags
    if (errno) {
        die(__LINE__, "%s: fcntl(), errno: %d", __func__, errno);
    }

    int rv = fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);  // modify and set the flags
    if (errno) {
        die(__LINE__, "%s: fcntl(), errno: %d", __func__, errno);
    }
    return rv;
}

static void handle_write(Conn *conn) {
    errno = 0;
    if (conn->outgoing.size() > 0) {
        ssize_t bytes = send(conn->fd, conn->outgoing.data(), conn->outgoing.size(), MSG_NOSIGNAL);
        if (bytes < 0 && errno == EAGAIN) {
            return;  // Socket not ready, will try later
        }
        if (bytes < 0) {
            msg(__LINE__, "%s: send() error", __func__);
            conn->want_close = true;  // error handling
            return;
        }

        // Remove written data
        buffer_consume(conn->outgoing, (size_t)bytes);

        // Update the readiness intention
        if (conn->outgoing.empty()) {  // all data written
            conn->want_read = true;
            conn->want_write = false;
        }  // else: want write
    }
}

static bool handle_client_request(Conn *conn) {
    // fprintf(stderr, "[DEBUG] Received raw data (size=%lu): ", conn->incoming.size());
    // for (size_t i = 0; i < conn->incoming.size(); i++) {
    //     fprintf(stderr, "%02X ", conn->incoming[i]);
    // }
    // fprintf(stderr, "\n");

    // while (!conn->incoming.empty() && conn->incoming[0] == 0) {
    //     buffer_consume(conn->incoming, 1);
    // }

    if (conn->incoming.size() < kHeaderSize) {
        fprintf(stderr, "[ERROR] Insufficient data for header parsing.\n");
        return false;
    }

    uint32_t len = 0;
    memcpy(&len, conn->incoming.data(), kHeaderSize);
    if (len > kMaxPayloadSize) {
        msg(__LINE__, "%s: Device %d: Invalid payload length. Supported: %ld, Received: %d",
            __func__, conn->fd, kMaxPayloadSize, len);
        conn->want_close = true;
        return false;
    }
    if (len == 0 || len > kMaxPayloadSize) {
        fprintf(stderr, "[ERROR] Invalid payload length (%u). Discarding request.\n", len);
        conn->want_close = true;
        return false;
    }
    if (conn->incoming.size() < kHeaderSize + len) {
        fprintf(stderr, "[DEBUG] Incomplete message, waiting for more data...\n");
        return false;
    }

    // Wait until we receive the full payload
    if (kHeaderSize + len > conn->incoming.size()) {
        return false;
    }

    std::vector<uint8_t> request(conn->incoming.begin() + kHeaderSize,
                                 conn->incoming.begin() + kHeaderSize + len);
    std::string command(request.begin(), request.end());
    fprintf(stderr, "[DEBUG] Client %d (len: %d) request: %s\n", conn->fd, len, command.c_str());

    // Process SET command
    if (command.compare(0, 4, "SET ") == 0) {
        size_t space_pos = command.find(' ', 4);
        if (space_pos != std::string::npos) {
            store_set_command(command.substr(4, space_pos - 4).c_str(),
                              command.substr(space_pos + 1).c_str());

            // Generate Response
            uint32_t response_len = 2;
            buffer_append(conn->outgoing, reinterpret_cast<const uint8_t *>(&response_len),
                          kHeaderSize);
            buffer_append(conn->outgoing, reinterpret_cast<const uint8_t *>("OK"), response_len);
        }
    }
    // Process GET command
    else if (command.compare(0, 4, "GET ") == 0) {
        const char *value = fetch_get_command(command.substr(4).c_str());

        // Generate Response
        const char *response = value ? value : "NULL";
        uint32_t response_len = strlen(response);
        buffer_append(conn->outgoing, reinterpret_cast<const uint8_t *>(&response_len),
                      kHeaderSize);
        buffer_append(conn->outgoing, reinterpret_cast<const uint8_t *>(response), response_len);
    } else {
        fprintf(stderr, "[ERROR] Invalid command from Client %d.\n", conn->fd);
        conn->want_close = true;
        return false;
    }

    // TODO: Don't we need to check if conn->outgoing is > 0 to confirm response exist?
    // Application logic is done, remove the request message
    buffer_consume(conn->incoming, kHeaderSize + len);
    conn->want_write = true;
    return true;
}

static void handle_read(Conn *conn) {
    uint8_t buf[64 * 1024];
    errno = 0;
    ssize_t bytes = recv(conn->fd, buf, sizeof(buf), MSG_DONTWAIT);
    if (bytes < 0 && errno == EAGAIN) {
        return;
    }

    // IO error
    if (bytes < 0) {
        msg(__LINE__, "%s: recv(), errno: %d", __func__, errno);
        conn->want_close = true;
        return;
    }

    // EOF
    if (bytes == 0) {
        if (conn->incoming.size() == 0) {
            fprintf(stderr, "[INFO] Client %d disconnected.\n", conn->fd);
        } else {
            fprintf(stderr, "[WARNING] Client %d disconnected unexpectedly with partial data.\n",
                    conn->fd);
        }
        conn->want_close = true;
        return;  // want close
    }

    // Ignore empty messages
    if (bytes == kHeaderSize && buf[0] == 0) {
        fprintf(stderr, "[WARNING] Client %d sent an empty request.\n", conn->fd);
        return;
    }

    buffer_append(conn->incoming, buf, (size_t)bytes);
    while (conn->incoming.size() >= kHeaderSize) {
        if (!handle_client_request(conn)) {
            break;
        }
    }

    // Update the readiness intention
    if (!conn->outgoing.empty()) {  // has a response
        conn->want_read = false;
        conn->want_write = true;
        // The socket is likely ready to write in a request-response protocol,
        // try to write it without waiting for the next iteration.
        return handle_write(conn);
    }  // else: want read
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
    rv = listen(server_fd, SOMAXCONN);  // BACKLOG
    if (rv) {
        die(__LINE__, "%s: listen(), errno: %d", __func__, errno);
    }

    printf("Server started on port %d, ready for GET/SET...\n", PORT);

    struct epoll_event event, events[MAX_EVENTS];
    int epoll_fd = epoll_create1(0);
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = server_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event);

    while (true) {
        int num_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        for (int i = 0; i < num_events; i++) {
            int fd = events[i].data.fd;
            if (fd == server_fd) {
                while (true) {  // Accept all pending connections
                    struct sockaddr_in client_addr = {};
                    socklen_t client_len = sizeof(client_addr);
                    int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
                    if (client_fd < 0) {
                        if (errno != EAGAIN && errno != EWOULDBLOCK) {
                            msg(__LINE__, "accept() error");
                        }
                        break;
                    }

                    set_nonblocking(client_fd);

                    if (fd2conn.size() <= (size_t)client_fd) {
                        fd2conn.resize(client_fd + 1);
                    }
                    if (fd2conn[client_fd] != nullptr) {
                        fprintf(stderr,
                                "[WARNING] Reusing file descriptor %d for new connection.\n",
                                client_fd);
                    }
                    fd2conn[client_fd] = new Conn{client_fd, true, false, false, {}, {}};
                    event.events = EPOLLIN | EPOLLET;
                    event.data.fd = client_fd;
                    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event);
                    printf("New client connected: %d from %s:%d\n", client_fd,
                           inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                }
            } else {
                // Handle request for existing client
                if (events[i].events == 0) {
                    continue;
                }

                Conn *conn = fd2conn[fd];
                if (events[i].events & EPOLLIN) {
                    handle_read(conn);
                }
                if (events[i].events & EPOLLOUT) {
                    handle_write(conn);
                }
                if (conn->want_close) {
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, conn->fd, nullptr);
                    close(conn->fd);
                    fd2conn[conn->fd] = nullptr;
                    delete conn;
                }
            }
        }
    }

    close(server_fd);
}
