#include "cacheX_client.hpp"

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
    char request[MAX_PAYLOAD_SIZE], response[MAX_PAYLOAD_SIZE];
    snprintf(request, sizeof(request), "SET %s %s", key, value);

    printf("[DEBUG] Sending request: %s\n", request);  // Debug print

    int result = send_request(sock, request);
    if (result < 0 || receive_response(sock, response, sizeof(response)) < 0) {
        perror("[ERROR] Failed to send SET command");
    }

    return result;
}

char *cacheX_get(int sock, const char *key) {
    char request[MAX_PAYLOAD_SIZE], response[MAX_PAYLOAD_SIZE];
    snprintf(request, sizeof(request), "GET %s", key);

    if (send_request(sock, request) < 0 || receive_response(sock, response, sizeof(response)) < 0) {
        return NULL;
    }

    return strdup(response);
}

void cacheX_close(int sock) { close(sock); }
