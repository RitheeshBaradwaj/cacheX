#ifndef SERVER_H_
#define SERVER_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PORT 6379        
#define MAX_EVENTS 10000  
#define BACKLOG 10000     
#define HASH_TABLE_SIZE 10007  // Prime number for better hashing

void start_server();
void handle_client_request(int client_fd);
void store_set_command(const char *key, const char *value);
const char *fetch_get_command(const char *key);

#ifdef __cplusplus
}
#endif

#endif  // SERVER_H_
