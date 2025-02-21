cmake_minimum_required(VERSION 3.10)
project(cacheX LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-O3 -Wall -Wextra -pedantic")

add_executable(cacheX src/main.cpp src/server.c)
target_include_directories(cacheX PRIVATE include)

# enable_testing()
# add_executable(test_server tests/test_server.cpp src/server.c)
# target_include_directories(test_server PRIVATE include)
# add_test(NAME TestServer COMMAND test_server)
