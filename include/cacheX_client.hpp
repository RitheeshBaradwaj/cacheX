#ifndef CACHEX_CLIENT_HPP_
#define CACHEX_CLIENT_HPP_

#include <string>

// Initialize the client connection
int cacheX_connect(const char *host, int port);

// Send a SET command
int cacheX_set(int sock, const std::string &key, const std::string &value);

// Send a GET command
std::string cacheX_get(int sock, const std::string &ey);

// Close the connection
void cacheX_close(int sock);

#endif  // CACHEX_CLIENT_HPPß_
