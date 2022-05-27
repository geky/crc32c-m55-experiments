// A crc32c implementation using polynomial remainder (expensive!),
// using multiplies instead of branches, a word at a time

#include <stdint.h>
#include <stddef.h>

uint32_t crc32c_naive_mul_32wide(
        uint32_t crc, const void *data, size_t size) {
    const uint8_t *data_ = data;
    crc = crc ^ 0xffffffff;

    for (size_t i = 0; i < size;) {
        if (((uintptr_t)&data_[i]) % 4 == 0 && i+4 <= size) {
            crc = crc ^ *(const uint32_t*)&data_[i];
            for (size_t j = 0; j < 32; j++) {
                crc = (crc >> 1) ^ ((crc & 1) * 0x82f63b78);
            }

            i += 4;
        } else {
            crc = crc ^ data_[i];
            for (size_t j = 0; j < 8; j++) {
                crc = (crc >> 1) ^ ((crc & 1) * 0x82f63b78);
            }

            i += 1;
        }
    }

    return crc ^ 0xffffffff;
}

