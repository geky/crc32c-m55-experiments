
// A crc32c implementation using a small 256 byte table

#include <stdint.h>
#include <stddef.h>

static const uint32_t TABLE[64] = {
    0x00000000, 0x105ec76f, 0x20bd8ede, 0x30e349b1,
    0x417b1dbc, 0x5125dad3, 0x61c69362, 0x7198540d,
    0x82f63b78, 0x92a8fc17, 0xa24bb5a6, 0xb21572c9,
    0xc38d26c4, 0xd3d3e1ab, 0xe330a81a, 0xf36e6f75,
};

uint32_t crc32c_small_table(uint32_t crc, const void *data, size_t size) {
    const uint8_t *data_ = data;
    crc ^= 0xffffffff;

    for (size_t i = 0; i < size; i++) {
        crc = (crc >> 4) ^ TABLE[0xf & (crc ^ (data_[i] >> 0))];
        crc = (crc >> 4) ^ TABLE[0xf & (crc ^ (data_[i] >> 4))];
    }

    return crc ^ 0xffffffff;
}

