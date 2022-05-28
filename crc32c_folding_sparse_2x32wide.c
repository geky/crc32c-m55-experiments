// A crc32c implementation using polynomial folding with pmul emulated
// using sparse multiplications, 2 words at a time

#include <stdint.h>
#include <stddef.h>


static inline uint32_t rbit32(uint32_t a) {
    uint32_t x;
    __asm__(
        "rbit %0,%1"
        : "=r"(x)
        : "r"(a)
    );
    return x;
}

static inline uint64_t pmul32(uint32_t a, uint32_t b) {
    uint64_t x = 0;
    for (size_t i = 0; i < 4; i++) {
        uint32_t a_ = a & (0x11111111 << i);
        for (size_t j = 0; j < 4; j++) {
            uint32_t b_ = b & (0x11111111 << (0x3 & (j-i)));
            x ^= (0x1111111111111111 << j) & ((uint64_t)a_ * (uint64_t)b_);
        }
    }
    return x;
}

uint32_t crc32c_folding_sparse_2x32wide(
        uint32_t crc, const void *data, size_t size) {
    const uint8_t *data_ = data;
    crc = crc ^ 0xffffffff;
    uint64_t folded = crc;

    for (size_t i = 0; i < size;) {
        if (((uintptr_t)&data_[i]) % 8 == 0 && i+8+8 <= size) {
            uint64_t d = folded ^ *(const uint64_t*)&data_[i];
            folded = pmul32((uint32_t)d, 0x493c7d27)
                   ^ pmul32((uint32_t)(d >> 32), 0xdd45aab8);
            i += 8;
        } else if (((uintptr_t)&data_[i]) % 4 == 0 && i+4 <= size) {
            crc = (uint32_t)folded ^ *(const uint32_t*)&data_[i];
            uint32_t b = (uint32_t)pmul32(crc, 0xdea713f1);
            folded = (folded >> 32)
                    ^ (uint32_t)(pmul32(b, 0x05ec76f1) >> 32)
                    ^ b;
            i += 4;
        } else {
            crc = (uint32_t)folded ^ data_[i];
            uint32_t b = (uint32_t)pmul32(crc << 24, 0xdea713f1);
            folded = (folded >> 8)
                    ^ (uint32_t)(pmul32(b, 0x05ec76f1) >> 32)
                    ^ b;
            i += 1;
        }
    }

    return (uint32_t)folded ^ 0xffffffff;
}

