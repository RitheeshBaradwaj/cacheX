#include "cacheX_client.hpp"

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <string>
#include <string_view>
#include <vector>

#include "cacheX_protocol.hpp"

int cacheX_connect(const char *host, int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) return -1;

    struct sockaddr_in server_addr = {};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    // convert IPv4 and IPv6 addresses from text to binary form
    inet_pton(AF_INET, host, &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        close(sock);
        return -1;
    }
    return sock;
}

int cacheX_set(int sock, const std::string &key, const std::string &value) {
    if (key.empty() || value.empty()) {
        printf("[ERROR] Key or Value cannot be empty.\n");
        return -1;
    }

    std::vector<std::string> command = {"SET", key, value};
    std::vector<uint8_t> response_buffer;
    if (send_request(sock, command) < 0 || receive_response(sock, response_buffer) < 0) {
        return -1;
    }

    print_response(response_buffer);
    return 0;
}

std::string cacheX_get(int sock, const std::string &key) {
    if (key.empty()) {
        return "[ERROR] Empty key!";
    }

    std::vector<std::string> command = {"GET", key};
    std::vector<uint8_t> response_buffer;
    if (send_request(sock, command) < 0 || receive_response(sock, response_buffer) < 0) {
        return "[ERROR] Failed to send GET command.";
    }

    print_response(response_buffer);
    if (response_buffer.size() > 8) {
        return std::string(response_buffer.begin() + 8, response_buffer.end());
    }
    return "";
}

void cacheX_close(int sock) {
    shutdown(sock, SHUT_RDWR);
    close(sock);
}
