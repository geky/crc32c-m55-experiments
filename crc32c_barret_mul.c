// A crc32c implementation using Barret reduction with a naive pmul,
// using multiplies instead of branches

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

uint32_t crc32c_barret_mul(
        uint32_t crc, const void *data, size_t size) {
    const uint8_t *data_ = data;
    crc = crc ^ 0xffffffff;

    for (size_t i = 0; i < size; i++) {
        crc = crc ^ data_[i];
        crc = (crc >> 8) ^ rbit32(pmul32(
            rbit32(pmul32(crc << 24, 0xdea713f1)),
            0x1edc6f41));
    }

    return crc ^ 0xffffffff;
}

