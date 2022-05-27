// A crc32c implementation using Barret reduction leveraging ARMv8-M's MVE
// vmull.p16 instruction

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
    uint16x8_t a_v = __arm_vdupq_n_u16(a);
    uint16x8_t b_v = __arm_vdupq_n_u16(b);
    a_v = __arm_vreinterpretq_u16_u32(
            __arm_vsetq_lane_u32(a, __arm_vreinterpretq_u32_u16(a_v), 1));
    b_v = __arm_vreinterpretq_u16_u32(
            __arm_vsetq_lane_u32(b, __arm_vreinterpretq_u32_u16(b_v), 2));

    uint32x4_t x_v = __arm_vmulltq_poly_p16(a_v, b_v);

    return __arm_vgetq_lane_u32(x_v, 0)
            ^ (__arm_vgetq_lane_u32(x_v, 1) << 16)
            ^ (__arm_vgetq_lane_u32(x_v, 2) << 16);
}

uint32_t crc32c_barret_vmullp16(
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

