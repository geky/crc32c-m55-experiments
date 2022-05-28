// A crc32c implementation using polynomial folding leveraging ARMv8-M's MVE
// vmull.p16 instruction, 2 words at a time

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

static inline uint64_t pmul32(uint32_t a, uint32_t b) {
    uint16x8_t a_v = __arm_vdupq_n_u16(a);
    uint16x8_t b_v = __arm_vdupq_n_u16(b);
    a_v = __arm_vreinterpretq_u16_u32(
            __arm_vsetq_lane_u32(a, __arm_vreinterpretq_u32_u16(a_v), 1));
    a_v = __arm_vreinterpretq_u16_u32(
            __arm_vsetq_lane_u32(a, __arm_vreinterpretq_u32_u16(a_v), 3));
    b_v = __arm_vreinterpretq_u16_u32(
            __arm_vsetq_lane_u32(b, __arm_vreinterpretq_u32_u16(b_v), 2));
    b_v = __arm_vreinterpretq_u16_u32(
            __arm_vsetq_lane_u32(b, __arm_vreinterpretq_u32_u16(b_v), 3));

    uint32x4_t x_v = __arm_vmulltq_poly_p16(a_v, b_v);

    return ((uint64_t)__arm_vgetq_lane_u32(x_v, 0))
         ^ ((uint64_t)__arm_vgetq_lane_u32(x_v, 1) << 16)
         ^ ((uint64_t)__arm_vgetq_lane_u32(x_v, 2) << 16)
         ^ ((uint64_t)__arm_vgetq_lane_u32(x_v, 3) << 32);
}

uint32_t crc32c_folding_vmullp16_2x32wide(
        uint32_t crc, const void *data, size_t size) {
    const uint8_t *data_ = data;
    crc = crc ^ 0xffffffff;
    uint64_t folded = crc;

    for (size_t i = 0; i < size;) {
        if (((uintptr_t)&data_[i]) % 8 == 0 && i+8+8 <= size) {
            uint64_t d = folded ^ *(const uint64_t*)&data_[i];
            folded = pmul32((uint32_t)d, 0x493c7d27)
                   ^ pmul32((uint32_t)(d >> 32), 0xdd45aab8);
            i += 8;
        } else if (((uintptr_t)&data_[i]) % 4 == 0 && i+4 <= size) {
            crc = (uint32_t)folded ^ *(const uint32_t*)&data_[i];
            uint32_t b = (uint32_t)pmul32(crc, 0xdea713f1);
            folded = (folded >> 32)
                    ^ (uint32_t)(pmul32(b, 0x05ec76f1) >> 32)
                    ^ b;
            i += 4;
        } else {
            crc = (uint32_t)folded ^ data_[i];
            uint32_t b = (uint32_t)pmul32(crc << 24, 0xdea713f1);
            folded = (folded >> 8)
                    ^ (uint32_t)(pmul32(b, 0x05ec76f1) >> 32)
                    ^ b;
            i += 1;
        }
    }

    return (uint32_t)folded ^ 0xffffffff;
}

