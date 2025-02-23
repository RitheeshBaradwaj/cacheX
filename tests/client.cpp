#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "cacheX_client.hpp"

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 6379
#define NUM_CONNECTIONS 50
#define MAX_BATCH 10

void* client_thread(void* arg) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr = {};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        return NULL;
    }

    struct timeval start, end;
    gettimeofday(&start, NULL);

    for (int i = 0; i < MAX_BATCH; i++) {
        char key[64], value[64];
        snprintf(key, sizeof(key), "key_%ld_%d", (long)arg, i);
        snprintf(value, sizeof(value), "value_%ld_%d", (long)arg, i);
        cacheX_set(sock, key, value);
    }

    gettimeofday(&end, NULL);

    long time_taken = ((end.tv_sec - start.tv_sec) * 1000000L) + (end.tv_usec - start.tv_usec);
    printf("[Thread %ld] Batch of %d requests took: %ld µs\n", (long)arg, MAX_BATCH, time_taken);

    close(sock);
    return NULL;
}

int main() {
    pthread_t threads[NUM_CONNECTIONS];

    struct timeval start, end;
    gettimeofday(&start, NULL);

    for (long i = 0; i < NUM_CONNECTIONS; i++) {
        pthread_create(&threads[i], NULL, client_thread, (void*)i);
    }
    for (int i = 0; i < NUM_CONNECTIONS; i++) {
        pthread_join(threads[i], NULL);
    }

    gettimeofday(&end, NULL);
    long total_time = ((end.tv_sec - start.tv_sec) * 1000000L) + (end.tv_usec - start.tv_usec);
    printf("\n[Total] %d connections (batch=%d) took: %ld µs (~%ld ms)\n", NUM_CONNECTIONS,
           MAX_BATCH, total_time, total_time / 1000);

    return 0;
}
