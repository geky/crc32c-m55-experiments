// A crc32c implementation using Barret reduction with a naive pmul,
// using multiplies instead of branches, a word at a time

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
    uint32_t x = 0;
    for (size_t i = 0; i < 32; i++) {
        x ^= (a & (1 << i)) * b;
    }
    return x;
}

uint32_t crc32c_barret_naive_mul_32wide(
        uint32_t crc, const void *data, size_t size) {
    const uint8_t *data_ = data;
    crc = crc ^ 0xffffffff;

    for (size_t i = 0; i < size;) {
        if (((uintptr_t)&data_[i]) % 4 == 0 && i+4 <= size) {
            crc = crc ^ *(const uint32_t*)&data_[i];
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

