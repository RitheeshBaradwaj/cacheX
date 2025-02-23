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

int cacheX_set(int sock, const char *key, const char *value) {
    std::string request = "SET ";
    request.append(key).append(" ").append(value);
    printf("[DEBUG] Sending request: %s\n", request.c_str());

    std::vector<uint8_t> req_buffer(request.begin(), request.end());
    int result = send_request(sock, req_buffer.data(), req_buffer.size());
    std::vector<uint8_t> response_buffer;
    response_buffer.resize(kMaxPayloadSize);
    if (result < 0 || receive_response(sock, response_buffer) < 0) {
        perror("[ERROR] Failed to send SET command");
    }

    return result;
}

std::string cacheX_get(int sock, const char *key) {
    if (!key || strlen(key) == 0) {
        return "ERROR: Empty key!";
    }

    std::string request = "GET ";
    request.append(key);
    printf("[DEBUG] Sending request: %s\n", request.c_str());

    std::vector<uint8_t> req_buffer(request.begin(), request.end());
    if (send_request(sock, req_buffer.data(), req_buffer.size()) < 0) {
        fprintf(stderr, "[ERROR] Failed to send GET request\n");
        return "ERROR";
    }

    std::vector<uint8_t> response_buffer;
    response_buffer.resize(kMaxPayloadSize);
    if (receive_response(sock, response_buffer) < 0) {
        return "ERROR";
    }

    // Convert response bytes to string and return
    return std::string(response_buffer.begin(), response_buffer.end());
}

void cacheX_close(int sock) {
    shutdown(sock, SHUT_RDWR);
    close(sock);
}
