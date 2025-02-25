#ifndef COMMON_HPP_
#define COMMON_HPP_

#include <stddef.h>
#include <stdint.h>

// typeof(((type *)0)->member) is used to determine the type of member in struct type.
// define container_of(ptr, type, member)
//     ({
//         const typeof(((type *)0)->member) *__mptr = (ptr);
//         (type *)((char *)__mptr - offsetof(type, member));
//     })

#define container_of(ptr, type, member) \
    reinterpret_cast<type*>(reinterpret_cast<char*>(ptr) - offsetof(type, member))

// Fowler-Noll-Vo hash
inline uint64_t str_hash(const uint8_t* data, size_t len) {
    uint32_t h = 0x811C9DC5;  // FNV-1a offset basis
    for (size_t i = 0; i < len; i++) {
        h = (h + data[i]) * 0x01000193;  // FNV prime multiplication
    }
    return h;
}

// Hash function (djb2 algorithm)
inline unsigned long hash_function(const char* str) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    return hash % HASH_TABLE_SIZE;
}

#endif  // COMMON_HPP_
