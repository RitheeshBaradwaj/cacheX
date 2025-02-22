#include "cacheX_client.h"

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int cacheX_connect(const char *host, int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) return -1;

    struct sockaddr_in server_addr = {0};
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
    char buffer[512];
    snprintf(buffer, sizeof(buffer), "SET %s %s", key, value);
    return send(sock, buffer, strlen(buffer), 0);
}

char *cacheX_get(int sock, const char *key) {
    char buffer[256], response[1024];
    snprintf(buffer, sizeof(buffer), "GET %s", key);
    send(sock, buffer, strlen(buffer), 0);

    ssize_t bytes = recv(sock, response, sizeof(response) - 1, 0);
    if (bytes > 0) {
        response[bytes] = '\0';
        return strdup(response);
    }
    return NULL;
}

void cacheX_close(int sock) { close(sock); }
