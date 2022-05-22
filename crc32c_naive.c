// A crc32c implementation using polynomial remainder (expensive!)

#include <stdint.h>
#include <stddef.h>

uint32_t crc32c_naive(uint32_t crc, const void *data, size_t size) {
    const uint8_t *data_ = data;
    crc = crc ^ 0xffffffff;

    for (size_t i = 0; i < size; i++) {
        crc = crc ^ data_[i];
        for (size_t j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ ((crc & 1) ? 0x82f63b78 : 0);
        }
    }

    return crc ^ 0xffffffff;
}

