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

enum ResponseStatus {
    RES_OK = 0,
    RES_ERR = 1,
    RES_NX = 2,  // Key not found
};

struct Response {
    uint32_t status = RES_OK;
    std::vector<uint8_t> data;
};

static inline int32_t read_all(int fd, void *buffer, size_t n) {
    size_t bytes_read = 0;
    errno = 0;
    while (bytes_read < n) {
        ssize_t result = recv(fd, (char *)buffer + bytes_read, n - bytes_read, 0);
        if (result <= 0) {
            fprintf(stderr, "Device %d: %s\n", fd, errno == 0 ? "EOF" : "recv() error");
            return -1;
        }
        bytes_read += result;
    }
    return 0;
}

static inline int32_t write_all(int fd, const void *buffer, size_t n) {
    size_t total_written = 0;
    while (total_written < n) {
        ssize_t bytes = send(fd, (char *)buffer + total_written, n - total_written, 0);
        if (bytes < 0) {
            if (errno == EINTR) {
                continue;  // Interrupted system call, retry
            } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
                fprintf(stderr, "Device %d: send() would block, retrying...\n", fd);
                continue;
            } else {
                fprintf(stderr, "Device %d: send() error: %s\n", fd, strerror(errno));
                return -1;
            }
        }

        total_written += bytes;
    }
    return 0;
}

// Append to the back
inline void buffer_append(std::vector<uint8_t> &buf, const uint8_t *data, size_t len) {
    buf.insert(buf.end(), data, data + len);
}

// Remove from the front
inline void buffer_consume(std::vector<uint8_t> &buf, size_t n) {
    buf.erase(buf.begin(), buf.begin() + n);
}

static inline int32_t send_request(int fd, const std::vector<std::string> &cmd) {
    uint32_t len = kHeaderSize;
    for (const std::string &s : cmd) {
        len += kHeaderSize + s.size();
    }
    if (len > kMaxPayloadSize) {
        return -1;
    }

    std::vector<uint8_t> wbuf;
    buffer_append(wbuf, reinterpret_cast<const uint8_t *>(&len), kHeaderSize);

    uint32_t n = cmd.size();
    buffer_append(wbuf, reinterpret_cast<const uint8_t *>(&n), kHeaderSize);

    for (const std::string &s : cmd) {
        uint32_t slen = s.size();
        buffer_append(wbuf, reinterpret_cast<const uint8_t *>(&slen), kHeaderSize);
        buffer_append(wbuf, reinterpret_cast<const uint8_t *>(s.data()), slen);
    }

    return write_all(fd, wbuf.data(), wbuf.size());
}

static inline int32_t receive_response(int fd, std::vector<uint8_t> &response_buffer) {
    uint32_t response_len = 0, status = 0;
    uint32_t header_size = kHeaderSize + sizeof(status);
    response_buffer.resize(header_size);

    if (read_all(fd, response_buffer.data(), sizeof(response_len)) < 0) {
        fprintf(stderr, "Device %d: Failed to read response length.\n", fd);
        return -1;
    }
    if (read_all(fd, response_buffer.data() + sizeof(response_len), sizeof(status)) < 0) {
        fprintf(stderr, "Device %d: Failed to read response status.\n", fd);
        return -1;
    }

    memcpy(&response_len, response_buffer.data(), sizeof(response_len));
    memcpy(&status, response_buffer.data() + sizeof(response_len), sizeof(status));

    uint32_t payload_size = response_len - sizeof(status);
    if (payload_size > 0) {
        response_buffer.resize(response_len + sizeof(status));
        if (read_all(fd, response_buffer.data() + sizeof(status) + kHeaderSize, payload_size) < 0) {
            fprintf(stderr, "Device %d: Failed to read response payload.\n", fd);
            return -1;
        }
    }

    printf("[DEBUG] Received response buffer (size=%ld): ", response_buffer.size());
    for (uint8_t byte : response_buffer) {
        printf("%02X ", byte);
    }
    printf("\n");

    return 0;
}

void print_response(const std::vector<uint8_t> &response_buffer) {
    fprintf(stderr, "[DEBUG] response_buffer (size=%lu): ", response_buffer.size());
    for (size_t i = 0; i < response_buffer.size(); i++) {
        fprintf(stderr, "%02X ", response_buffer[i]);
    }
    fprintf(stderr, "\n");
    if (response_buffer.size() < 8) {
        fprintf(stderr, "[ERROR] Invalid response received: %ld (too short).\n",
                response_buffer.size());
        return;
    }

    uint32_t response_length = 0;
    memcpy(&response_length, response_buffer.data(), sizeof(uint32_t));

    if (response_length != response_buffer.size() - sizeof(uint32_t)) {
        fprintf(stderr, "[ERROR] Mismatched response length.\n");
        return;
    }

    uint32_t status = 0;
    memcpy(&status, response_buffer.data() + sizeof(uint32_t), sizeof(uint32_t));

    std::string status_str;
    switch (status) {
        case RES_OK:
            status_str = "OK";
            break;
        case RES_ERR:
            status_str = "ERROR";
            break;
        case RES_NX:
            status_str = "NOT FOUND";
            break;
        default:
            status_str = "UNKNOWN";
            break;
    }

    std::string data;
    if (response_buffer.size() > 8) {
        data.assign(reinterpret_cast<const char *>(response_buffer.data() + 8),
                    response_buffer.size() - 8);
    }

    fprintf(stderr, "[RESPONSE] Status: %s\n", status_str.c_str());
    if (!data.empty()) {
        fprintf(stderr, "[RESPONSE] Data: %s\n", data.c_str());
    }
}

static inline bool read_u32(const uint8_t *&cur, const uint8_t *end, uint32_t &out) {
    if (cur + kHeaderSize > end) {
        return false;
    }
    memcpy(&out, cur, kHeaderSize);
    cur += kHeaderSize;
    return true;
}

static inline bool read_str(const uint8_t *&cur, const uint8_t *end, size_t len, std::string &out) {
    if (cur + len > end) {
        return false;
    }
    out.assign(reinterpret_cast<const char *>(cur), len);
    cur += len;
    return true;
}

// +------+-----+------+-----+------+-----+-----+------+
// | nstr | len | str1 | len | str2 | ... | len | strn |
// +------+-----+------+-----+------+-----+-----+------+
int32_t parse_request(const uint8_t *data, size_t size, std::vector<std::string> &out) {
    const uint8_t *end = data + size;
    uint32_t nstr = 0;

    if (!read_u32(data, end, nstr) || nstr > kMaxArgs) {
        return -1;  // Malformed request
    }

    while (out.size() < nstr) {
        uint32_t len = 0;
        if (!read_u32(data, end, len)) {
            return -1;
        }
        out.emplace_back();
        if (!read_str(data, end, len, out.back())) {
            return -1;
        }
    }

    // Ensure no extra garbage data
    return (data == end) ? 0 : -1;
}

inline void create_response(const Response &resp, std::vector<uint8_t> &out) {
    uint32_t resp_len = kHeaderSize + static_cast<uint32_t>(resp.data.size());
    uint32_t status = resp.status;

    buffer_append(out, reinterpret_cast<const uint8_t *>(&resp_len), kHeaderSize);
    buffer_append(out, reinterpret_cast<const uint8_t *>(&status), kHeaderSize);
    buffer_append(out, resp.data.data(), resp.data.size());
}

#endif  // CACHEX_PROTOCOL_HPP_
