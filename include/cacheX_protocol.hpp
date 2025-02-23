#ifndef CACHEX_PROTOCOL_HPP_
#define CACHEX_PROTOCOL_HPP_

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

constexpr size_t kHeaderSize = 4;
constexpr size_t kMaxPayloadSize = 32 * 1024 * 1024;  // 32 << 20 (32MB)

struct CacheXMessage {
    uint32_t length;                // Payload size
    char payload[kMaxPayloadSize];  // Data buffer
};

static inline int32_t read_all(int fd, char *buf, size_t n) {
    size_t total_read = 0;
    while (total_read < n) {
        ssize_t bytes = recv(fd, buf + total_read, n - total_read, 0);
        if (bytes <= 0) return -1;  // error, or unexpected EOF
        total_read += bytes;
    }
    return 0;
}

static inline int32_t write_all(int fd, const char *buf, size_t n) {
    size_t total_written = 0;
    while (total_written < n) {
        ssize_t bytes = send(fd, buf + total_written, n - total_written, 0);
        if (bytes <= 0) return -1;  // error
        total_written += bytes;
    }
    return 0;
}

static inline void encode_request(CacheXMessage *msg, const char *data) {
    msg->length = strlen(data);
    if (msg->length > kMaxPayloadSize) {
        // TODO: Reject the request!!
        fprintf(stderr, "Payload too large! Truncating to %ld bytes.\n", kMaxPayloadSize);
        msg->length = kMaxPayloadSize;
    }
    memcpy(msg->payload, data, msg->length);
}

static inline uint32_t decode_response(CacheXMessage *msg, char *output, size_t max_output_size) {
    size_t len = msg->length;

    if (len >= max_output_size) {  // Prevent overflow
        fprintf(stderr, "Received payload too large! Truncating.\n");
        len = max_output_size - 1;
    }

    memcpy(output, msg->payload, len);
    output[len] = '\0';
    return len;
}

static inline int32_t send_request(int fd, const char *data) {
    CacheXMessage request;
    encode_request(&request, data);

    char buffer[kHeaderSize + kMaxPayloadSize];
    memcpy(buffer, &request.length, kHeaderSize);
    memcpy(buffer + kHeaderSize, request.payload, request.length);

    errno = 0;
    int32_t err = write_all(fd, buffer, kHeaderSize + request.length);
    if (err) {
        fprintf(stderr, "Device %d: %s\n", fd, errno == 0 ? "EOF" : "write() error");
        return err;
    }

    return err;
}

static inline int32_t receive_response(int fd, char *output, size_t max_output_size) {
    CacheXMessage response;
    errno = 0;

    // Read response header
    int32_t err = read_all(fd, (char *)&response.length, kHeaderSize);
    if (err) {
        fprintf(stderr, "Device %d: %s\n", fd, errno == 0 ? "EOF" : "read() error");
        return err;
    }

    if (response.length > kMaxPayloadSize) {
        // response.length = kMaxPayloadSize;
        fprintf(stderr, "Device %d: Invalid payload length. Supported: %ld, Received: %u\n", fd,
                kMaxPayloadSize, response.length);
        return -1;
    }

    // Read response payload
    err = read_all(fd, response.payload, response.length);
    if (err) {
        fprintf(stderr, "read() error\n");
        return err;
    }

    return decode_response(&response, output, max_output_size);
}

#endif  // CACHEX_PROTOCOL_HPP_
