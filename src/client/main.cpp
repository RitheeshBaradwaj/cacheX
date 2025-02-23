#include <iostream>
#include <string>

#include "cacheX_client.hpp"

void print_usage() {
    std::cout << "\nAvailable commands:\n"
              << "  SET <key> <value>  - Store a value\n"
              << "  GET <key>         - Retrieve a value\n"
              << "  EXIT              - Close connection\n\n";
}

int main() {
    int sock = cacheX_connect("127.0.0.1", 6379);
    if (sock == -1) {
        std::cerr << "Failed to connect to CacheX server\n";
        return EXIT_FAILURE;
    }

    std::cout << "Connected to cacheX server. Type 'HELP' for available commands.\n";

    std::string input, key, value;
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, input);

        if (input.empty()) continue;

        if (input == "EXIT") break;
        if (input == "HELP") {
            print_usage();
            continue;
        }

        size_t space_pos = input.find(' ');
        if (space_pos != std::string::npos) {
            std::string command = input.substr(0, space_pos);
            std::string rest = input.substr(space_pos + 1);

            if (command == "SET") {
                size_t value_pos = rest.find(' ');
                if (value_pos != std::string::npos) {
                    key = rest.substr(0, value_pos);
                    value = rest.substr(value_pos + 1);

                    int rv = cacheX_set(sock, key.c_str(), value.c_str());
                    if (rv < 0) {
                        std::cerr << "Error: Failed to send SET command\n";
                    } else {
                        std::cout << "OK\n";
                    }
                } else {
                    std::cerr << "Invalid SET command format. Use: SET <key> <value>\n";
                }
            } else if (command == "GET") {
                key = rest;
                std::string result = cacheX_get(sock, key.c_str());
                if (result != "ERROR") {
                    std::cout << "Server: " << result << "\n";
                } else {
                    std::cerr << "Error: Failed to retrieve key or key does not exist\n";
                }
            } else {
                std::cerr << "Invalid command. Type 'HELP' for available commands.\n";
            }
        } else {
            std::cerr << "Invalid command. Type 'HELP' for available commands.\n";
        }
    }

    cacheX_close(sock);
    return EXIT_SUCCESS;
}
