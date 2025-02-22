#ifndef SERVER_HPP_
#define SERVER_HPP_

#include <stdint.h>

#define PORT 6379
#define BACKLOG 10000
#define MAX_EVENTS 10000
#define HASH_TABLE_SIZE 10007  // Prime number for better hashing

void start_server();

void store_set_command(const char *key, const char *value);
const char *fetch_get_command(const char *key);

#endif  // SERVER_HPP_
