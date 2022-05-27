// A crc32c implementation using Barret reduction with pmul emulated
// using sparse multiplications

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
    for (size_t i = 0; i < 4; i++) {
        uint32_t a_ = a & (0x11111111 << i);
        for (size_t j = 0; j < 4; j++) {
            uint32_t b_ = b & (0x11111111 << (0x3 & (j-i)));
            x ^= (0x11111111 << j) & (a_ * b_);
        }
    }
    return x;
}

uint32_t crc32c_barret_sparse(
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

