cmake_minimum_required(VERSION 3.10)
project(cacheX LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-O3 -Wall -Wextra -pedantic")

add_executable(cacheX src/main.cpp src/server.c)
target_include_directories(cacheX PRIVATE include)

add_executable(cacheX_client src/client/cacheX_client.c)

enable_testing()
add_executable(test_client tests/client.c src/server.c)
target_include_directories(test_client PRIVATE include)
add_test(NAME TestServer COMMAND test_client)
