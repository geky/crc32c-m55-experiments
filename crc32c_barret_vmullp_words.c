// A crc32c implementation using Barret reduction leveraging ARMv8-M's MVE
// vmull.p16 instruction, a word at a time

#include <stdint.h>
#include <stddef.h>

#include <arm_mve.h>


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
    uint16x8_t a_ = __arm_vdupq_n_u16(a);
    uint16x8_t b_ = __arm_vdupq_n_u16(b);
    a_ = __arm_vreinterpretq_u16_u32(
            __arm_vsetq_lane_u32(a, __arm_vreinterpretq_u32_u16(a_), 1));
    b_ = __arm_vreinterpretq_u16_u32(
            __arm_vsetq_lane_u32(b, __arm_vreinterpretq_u32_u16(b_), 2));

    uint32x4_t x_ = __arm_vmulltq_poly_p16(a_, b_);

    return __arm_vgetq_lane_u32(x_, 0)
            ^ (__arm_vgetq_lane_u32(x_, 1) << 16)
            ^ (__arm_vgetq_lane_u32(x_, 2) << 16);
}

uint32_t crc32c_barret_vmullp_words(
        uint32_t crc, const void *data, size_t size) {
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

