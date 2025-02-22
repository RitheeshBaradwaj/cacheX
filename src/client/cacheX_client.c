#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 6379

void start_client() {
    int sock;
    struct sockaddr_in server_addr;
    char input[512], response[1024];

    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("Invalid address");
        exit(EXIT_FAILURE);
    }

    // Connect to cacheX server
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    printf("Connected to cacheX server at %s:%d\n", SERVER_IP, SERVER_PORT);
    printf("Enter commands (SET key value | GET key | EXIT):\n");

    while (1) {
        printf("> ");
        fflush(stdout);

        if (!fgets(input, sizeof(input), stdin)) {
            break;
        }

        // Remove newline character
        input[strcspn(input, "\n")] = 0;

        // Exit command
        if (strcmp(input, "EXIT") == 0) {
            printf("Exiting client...\n");
            break;
        }

        // Send command to server
        send(sock, input, strlen(input), 0);

        // Receive response
        ssize_t bytes_received = recv(sock, response, sizeof(response) - 1, 0);
        if (bytes_received > 0) {
            response[bytes_received] = '\0';
            printf("Server: %s\n", response);
        } else {
            printf("Server disconnected.\n");
            break;
        }
    }

    // Close the connection
    close(sock);
}

int main() {
    start_client();
    return 0;
}
