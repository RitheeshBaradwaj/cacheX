#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>

#include "cacheX_client.hpp"

void print_usage() {
    std::cout << "\nAvailable commands:\n"
              << "  SET <key> <value>  - Store a value\n"
              << "  GET <key>          - Retrieve a value\n"
              << "  EXIT               - Close connection\n\n";
}

void handle_set(int sock, std::istringstream &iss) {
    std::string key, value;
    if (!(iss >> key) || !(std::getline(iss >> std::ws, value))) {
        std::cerr << "[ERROR] Invalid SET format. Use: SET <key> <value>\n";
        return;
    }

    if (cacheX_set(sock, key, value) < 0) {
        std::cerr << "[ERROR] Failed to send SET command.\n";
    } else {
        std::cout << "OK\n";
    }
}

void handle_get(int sock, std::istringstream &iss) {
    std::string key;
    if (!(iss >> key)) {
        std::cerr << "[ERROR] Invalid GET format. Use: GET <key>\n";
        return;
    }

    std::string result = cacheX_get(sock, key);
    if (!result.empty() && result != "ERROR") {
        std::cout << "[RESPONSE] " << result << "\n";
    } else {
        std::cerr << "[ERROR] Key not found or request failed.\n";
    }
}

int main() {
    int sock = cacheX_connect("127.0.0.1", 6379);
    if (sock == -1) {
        std::cerr << "[ERROR] Failed to connect to CacheX server.\n";
        return EXIT_FAILURE;
    }

    std::cout << "Connected to CacheX server. Type 'HELP' for available commands.\n";

    std::unordered_map<std::string, std::function<void(int, std::istringstream &)>> command_map = {
        {"SET", handle_set},
        {"GET", handle_get},
        {"HELP", [](int, std::istringstream &) { print_usage(); }},
        {"EXIT", [](int, std::istringstream &) { std::cout << "Exiting...\n"; }}};

    std::string input;
    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, input) || input.empty()) {
            continue;
        }

        std::istringstream iss(input);
        std::string command;
        if (!(iss >> command)) continue;

        auto it = command_map.find(command);
        if (it != command_map.end()) {
            if (command == "EXIT") break;
            it->second(sock, iss);
        } else {
            std::cerr << "[ERROR] Invalid command. Type 'HELP' for available commands.\n";
        }
    }

    cacheX_close(sock);
    return EXIT_SUCCESS;
}
