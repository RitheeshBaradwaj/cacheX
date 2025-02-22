cmake_minimum_required(VERSION 3.10)
project(cacheX LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-O3 -Wall -Wextra -pedantic")

# Build the cacheX server
add_executable(cacheX src/main.cpp src/server.c)
target_include_directories(cacheX PRIVATE include)

# Build the client library (shared and static)
add_library(cacheX_client SHARED src/client/cacheX_client.c)
add_library(cacheX_client_static STATIC src/client/cacheX_client.c)
target_include_directories(cacheX_client PUBLIC include)
target_include_directories(cacheX_client_static PUBLIC include)  # Fix added here

# Build the standalone client binary
add_executable(cacheX_cli src/client/main.c)
target_link_libraries(cacheX_cli cacheX_client)
target_include_directories(cacheX_cli PRIVATE include)

# Install rules
install(TARGETS cacheX_client cacheX_client_static cacheX_cli
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)
install(FILES include/cacheX_client.h DESTINATION include)

# Enable testing
enable_testing()
add_executable(test_client tests/client.c src/server.c)
target_include_directories(test_client PRIVATE include)
add_test(NAME TestServer COMMAND test_client)
