#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 6379
#define NUM_CONNECTIONS 10000

void* client_thread(void* arg) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr = {.sin_family = AF_INET, .sin_port = htons(SERVER_PORT)};
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);
    connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr));

    char message[256];
    sprintf(message, "SET key_%ld value_%ld", (long)arg, (long)arg);
    send(sock, message, strlen(message), 0);
    close(sock);
    return NULL;
}

int main() {
    pthread_t threads[NUM_CONNECTIONS];
    for (long i = 0; i < NUM_CONNECTIONS; i++) {
        pthread_create(&threads[i], NULL, client_thread, (void*)i);
    }
    for (int i = 0; i < NUM_CONNECTIONS; i++) {
        pthread_join(threads[i], NULL);
    }
    return 0;
}
