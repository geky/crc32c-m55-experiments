// A crc32c implementation using polynomial folding leveraging ARMv8-M's MVE
// vmull.p16 instruction, 8 16-bit halfwords at a time

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
    a_v = (uint16x8_t)__arm_vsetq_lane_u32(a, (uint32x4_t)a_v, 1);
    b_v = (uint16x8_t)__arm_vsetq_lane_u32(b, (uint32x4_t)b_v, 2);

    uint32x4_t x_v = __arm_vmulltq_poly_p16(a_v, b_v);

    return __arm_vgetq_lane_u32(x_v, 0)
            ^ (__arm_vgetq_lane_u32(x_v, 1) << 16)
            ^ (__arm_vgetq_lane_u32(x_v, 2) << 16);
}

uint32_t crc32c_folding_vmullp16_8x16wide(
        uint32_t crc, const void *data, size_t size) {
    const uint8_t *data_ = data;
    crc = crc ^ 0xffffffff;

    uint32x4_t k_lower_v = __arm_vdupq_n_u32(0x55460dfe);
    uint32x4_t k_upper_v = __arm_vdupq_n_u32(0x5407f20c);

    uint32x4_t folded_v = __arm_vsetq_lane_u32(crc, __arm_vdupq_n_u32(0), 0);
    uint32_t overflow = 0;

    for (size_t i = 0; i < size;) {
        if (((uintptr_t)&data_[i]) % 16 == 0 && i+16+16 <= size) {
            // xor data into folded
            folded_v = __arm_veorq_u32(folded_v,
                    __arm_vld1q_u32((const uint32_t*)&data_[i]));
            // 2x p16xp32 -> p48 folds
            uint32x4_t lower0_v = __arm_vmullbq_poly_p16(
                    (uint16x8_t)folded_v, (uint16x8_t)k_lower_v);
            uint32x4_t lower1_v = __arm_vmulltq_poly_p16(
                    (uint16x8_t)folded_v, (uint16x8_t)k_lower_v);
            uint32x4_t upper0_v = __arm_vmullbq_poly_p16(
                    (uint16x8_t)folded_v, (uint16x8_t)k_upper_v);
            uint32x4_t upper1_v = __arm_vmulltq_poly_p16(
                    (uint16x8_t)folded_v, (uint16x8_t)k_upper_v);
            // xor/shift into folded
            uint32x4_t lower_v = __arm_veorq_u32(lower0_v, lower1_v);
            uint32x4_t upper_v = __arm_veorq_u32(upper0_v, upper1_v);
            folded_v = __arm_veorq_u32(lower_v,
                    __arm_vshlcq_u32(upper_v, &overflow, 16));
            i += 16;
        } else if (((uintptr_t)&data_[i]) % 4 == 0 && i+4 <= size) {
            // Barret reduce 32-bit words
            crc = __arm_vgetq_lane_u32(folded_v, 0)
                    ^ *(const uint32_t*)&data_[i];
            crc = __arm_vgetq_lane_u32(folded_v, 1) ^ rbit32(pmul32(
                    rbit32(pmul32(crc, 0xdea713f1)),
                    0x1edc6f41));
            folded_v = __arm_vsetq_lane_u32(crc, folded_v, 0);
            folded_v = __arm_vsetq_lane_u32(
                    __arm_vgetq_lane_u32(folded_v, 2), folded_v, 1);
            folded_v = __arm_vsetq_lane_u32(
                    __arm_vgetq_lane_u32(folded_v, 3), folded_v, 2);
            folded_v = __arm_vsetq_lane_u32(overflow, folded_v, 3);
            overflow = 0;
            i += 4;
        } else {
            // Barret reduce 8-bit bytes
            crc = __arm_vgetq_lane_u32(folded_v, 0) ^ data_[i];
            crc = (crc >> 8) ^ rbit32(pmul32(
                    rbit32(pmul32(crc << 24, 0xdea713f1)),
                    0x1edc6f41));
            folded_v = __arm_vsetq_lane_u32(crc, folded_v, 0);
            i += 1;
        }
    }

    return __arm_vgetq_lane_u32(folded_v, 0) ^ 0xffffffff;
}

