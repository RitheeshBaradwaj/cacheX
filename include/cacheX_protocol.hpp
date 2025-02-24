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
constexpr size_t kMaxArgs = 200 * 1000;

static inline int32_t read_all(int fd, uint8_t *buf, size_t n) {
    size_t total_read = 0;
    errno = 0;
    while (total_read < n) {
        ssize_t bytes = recv(fd, buf + total_read, n - total_read, MSG_DONTWAIT);
        if (bytes <= 0) {
            fprintf(stderr, "Device %d: %s\n", fd, errno == 0 ? "EOF" : "read() error");
            return -1;
        }
        total_read += bytes;
    }
    return 0;
}

static inline int32_t write_all(int fd, const uint8_t *buf, size_t n) {
    size_t total_written = 0;
    errno = 0;
    while (total_written < n) {
        ssize_t bytes = send(fd, buf + total_written, n - total_written, 0);
        if (bytes <= 0) {
            fprintf(stderr, "Device %d: %s\n", fd, errno == 0 ? "EOF" : "write() error");
            return -1;
        }
        total_written += bytes;
    }
    return 0;
}

// Append to the back
static inline void buffer_append(std::vector<uint8_t> &buf, const uint8_t *data, size_t len) {
    buf.insert(buf.end(), data, data + len);
}

// Remove from the front
inline void buffer_consume(std::vector<uint8_t> &buf, size_t n) {
    buf.erase(buf.begin(), buf.begin() + n);
}

static inline int32_t send_request(int fd, const uint8_t *req, size_t len) {
    if (len > kMaxPayloadSize) {
        return -1;
    }
    std::vector<uint8_t> wbuf;
    buffer_append(wbuf, (const uint8_t *)&len, kHeaderSize);
    buffer_append(wbuf, req, len);
    return write_all(fd, wbuf.data(), wbuf.size());
}

static inline int32_t receive_response(int fd, std::vector<uint8_t> &response_buffer) {
    // 4-byte header to read payload length
    uint32_t len = 0;
    if (read_all(fd, reinterpret_cast<uint8_t *>(&len), kHeaderSize) < 0) {
        fprintf(stderr, "Device %d: Failed to read response header.\n", fd);
        return -1;
    }

    if (len > kMaxPayloadSize) {
        fprintf(stderr, "Device %d: Invalid payload length. Supported: %ld, Received: %u\n", fd,
                kMaxPayloadSize, len);
        return -1;
    }
    response_buffer.resize(len);
    return read_all(fd, response_buffer.data(), len);
}

#endif  // CACHEX_PROTOCOL_HPP_
