// A crc32c implementation using Barret reduction with pmul emulated
// using sparse multiplications, loop unrolled

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
    uint32_t x = 0;
    for (size_t i = 0; i < 4; i++) {
        x ^= (0x11111111 << i) & (
              a0 * (b & (0x11111111 << (0x3 & (0+i))))
            ^ a1 * (b & (0x11111111 << (0x3 & (3+i))))
            ^ a2 * (b & (0x11111111 << (0x3 & (2+i))))
            ^ a3 * (b & (0x11111111 << (0x3 & (1+i)))));
    }
    return x;
}

uint32_t crc32c_barret_sparse_semirolled(
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

