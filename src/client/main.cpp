#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cacheX_client.h"

int main() {
    int sock = cacheX_connect("127.0.0.1", 6379);
    if (sock == -1) {
        printf("Failed to connect to cacheX server\n");
        return EXIT_FAILURE;
    }

    printf("Connected to cacheX server. Enter commands (SET key value | GET key | EXIT):\n");

    char command[512], key[256], value[256];
    while (1) {
        printf("> ");
        fflush(stdout);
        if (!fgets(command, sizeof(command), stdin)) {
            printf("Error reading input. Exiting...\n");
            break;
        }
        command[strcspn(command, "\n")] = 0;

        if (strcmp(command, "EXIT") == 0) break;
        if (sscanf(command, "SET %255s %255[^\n]", key, value) == 2) {
            cacheX_set(sock, key, value);
            printf("OK\n");
        } else if (sscanf(command, "GET %255s", key) == 1) {
            char *result = cacheX_get(sock, key);
            printf("Server: %s\n", result ? result : "NULL");
            free(result);
        } else {
            printf("Invalid command\n");
        }
    }

    cacheX_close(sock);
    return EXIT_SUCCESS;
}
