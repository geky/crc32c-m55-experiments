// A crc32c implementation using Barret reduction with pmul emulated
// using sparse multiplications, a word at a time

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

static inline uint32_t pmul32(uint32_t a, uint32_t b) {
    uint32_t a0 = a & 0x11111111;
    uint32_t a1 = a & 0x22222222;
    uint32_t a2 = a & 0x44444444;
    uint32_t a3 = a & 0x88888888;
    uint32_t b0 = b & 0x11111111;
    uint32_t b1 = b & 0x22222222;
    uint32_t b2 = b & 0x44444444;
    uint32_t b3 = b & 0x88888888;
    return (0x11111111 & ((a0*b0) ^ (a1*b3) ^ (a2*b2) ^ (a3*b1)))
         | (0x22222222 & ((a0*b1) ^ (a1*b0) ^ (a2*b3) ^ (a3*b2)))
         | (0x44444444 & ((a0*b2) ^ (a1*b1) ^ (a2*b0) ^ (a3*b3)))
         | (0x88888888 & ((a0*b3) ^ (a1*b2) ^ (a2*b1) ^ (a3*b0)));
}

uint32_t crc32c_barret_sparse_words(uint32_t crc, const void *data, size_t size) {
    const uint8_t *data_ = data;
    crc = crc ^ 0xffffffff;

    for (size_t i = 0; i < size;) {
        if (((uintptr_t)&data_[i]) % 4 == 0 && i+4 <= size) {
            crc = crc ^ ((const uint32_t*)data_)[i/4];
            crc = rbit32(pmul32(
                rbit32(pmul32(crc, 0xdea713f1)),
                0x1edc6f41));
            i += 4;
        } else {
            crc = crc ^ data_[i];
            crc = (crc >> 8) ^ rbit32(pmul32(
                rbit32(pmul32(crc << 24, 0xdea713f1)),
                0x1edc6f41));
            i += 1;
        }
    }

    return crc ^ 0xffffffff;
}

