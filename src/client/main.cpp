#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cacheX_client.hpp"

void print_usage() {
    printf("\nAvailable commands:\n");
    printf("  SET <key> <value>  - Store a value\n");
    printf("  GET <key>         - Retrieve a value\n");
    printf("  EXIT              - Close connection\n\n");
}

int main() {
    int sock = cacheX_connect("127.0.0.1", 6379);
    if (sock == -1) {
        printf("Failed to connect to cacheX server\n");
        return EXIT_FAILURE;
    }

    printf("Connected to cacheX server. Type 'HELP' for available commands.\n");

    char command[512], key[256], value[256];
    while (true) {
        printf("> ");
        fflush(stdout);
        if (!fgets(command, sizeof(command), stdin)) {
            printf("Error reading input. Exiting...\n");
            break;
        }
        command[strcspn(command, "\n")] = '\0';  // Ensure proper null termination
        if (strlen(command) == 0) continue;      // Ignore empty input

        if (strcmp(command, "EXIT") == 0) break;
        if (strcmp(command, "HELP") == 0) {
            print_usage();
            continue;
        }
        if (sscanf(command, "SET %255s %255[^\n]", key, value) == 2) {
            int rv = cacheX_set(sock, key, value);
            if (rv < 0) {
                printf("Error: Failed to send SET command\n");
            } else {
                printf("OK\n");
            }
        } else if (sscanf(command, "GET %255s", key) == 1) {
            char *result = cacheX_get(sock, key);
            if (result) {
                printf("Server: %s\n", result);
                free(result);
            } else {
                printf("Error: Failed to retrieve key or key does not exist\n");
            }
        } else {
            printf("Invalid command. Type 'HELP' for available commands.\n");
        }
    }

    cacheX_close(sock);
    return EXIT_SUCCESS;
}
