#ifndef CACHEX_CLIENT_H_
#define CACHEX_CLIENT_H_

// Initialize the client connection
int cacheX_connect(const char *host, int port);

// Send a SET command
int cacheX_set(int sock, const char *key, const char *value);

// Send a GET command
char *cacheX_get(int sock, const char *key);

// Close the connection
void cacheX_close(int sock);

#endif  // CACHEX_CLIENT_H_
