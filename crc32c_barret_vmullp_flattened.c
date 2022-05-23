// A crc32c implementation using Barret reduction leveraging ARMv8-M's MVE
// vmull.p16 instruction, flattened and attempted hand-optimized

//
// From a high-level, the core of CRC using Barret reduction is pretty simple:
//
// crc = (crc >> n) ^ hi32(pmul32(
//     lo32(pmul32(crc << n, <barret constant>)),
//     <crc polynomial>))
//
// Unfortunately, things get a bit messy in MVE:
//
// 1. MVE supports only 4 x 16-bit pmul or 8 x 8-bit pmul. Though since MVE is
//    a SIMD instruction set, we can do all parts of the 32-bit long
//    multiplication in a single instruction.
//
// 2. MVE doesn't have a reducing xor instruction, making the final summation of
//    long multiplication a bit annoying.
//
// With barret reduction, you only need half of each multiword-multiply, but
// unfortunately due to data dependencies this doesn't really buy us much.
//
// = hi32(lo32(a * c_b) * c_p)
//
//        .- lo16(hi16(a)*lo16(c_b)):0u16 + lo16(lo16(a)*hi16(c_b)):0u16 + lo16(a)*lo16(c_b)
//        v
// = hi32(_ * c_p)
//
//        .-----------------------------+------------------------------+- lo16(hi16(a)*lo16(c_b)):0u16 + lo16(lo16(a)*hi16(c_b)):0u16 + lo16(a)*lo16(c_b)
//        v                             v                              v
// = hi16(_)*hi16(c_p) + sr16_lo16(hi16(_)*lo16(c_p)) + sr16_lo16(lo16(_)*hi16(c_p))
//
//        .------------------------+- lo16(hi16(a)*lo16(c_b)):0u16 + lo16(lo16(a)*hi16(c_b)):0u16 + lo16(a)*lo16(c_b)
//        |                        |                                                                       |
//        |                        |                         .---------------------------------------------'
//        v                        v                         v
// = hi16(_)*hi16(c_p) + hi16(hi16(_)*lo16(c_p)) + hi16(lo16(_)*hi16(c_p))
//
//

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

uint32_t crc32c_barret_vmullp_flattened(
        uint32_t crc, const void *data, size_t size) {
    const uint8_t *data_ = data;
    crc = crc ^ 0xffffffff;

    // prepare our Barret/polynomial constants for lane-wise pmul
    uint16x8_t c_b_ = __arm_vcreateq_u16(
            0x13f10000dea70000, 0x13f1000013f10000);
    uint16x8_t c_p_ = __arm_vcreateq_u16(
            0x000082f600003b78, 0x00003b78000082f6);

    for (size_t i = 0; i < size; i++) {
        crc = crc ^ data_[i];

        // pmul(crc << 24, <barret constant>)
        uint16x8_t a_ = __arm_vreinterpretq_u16_u32(
                __arm_vdupq_n_u32(crc << 24));
        uint16x8_t x_ = __arm_vreinterpretq_u16_u32(
                __arm_vmulltq_poly_p16(a_, c_b_));

        // pmull(lo32(_), <polynomial constant>)
        uint32x4_t y_ = __arm_vmullbq_poly_p16(x_, c_p_);

        // hi32(_)
        y_ = __arm_vshlq_n_u32(y_, 1);
        crc = (crc >> 8)
                ^ __arm_vgetq_lane_u32(y_, 3)
                ^ __arm_vgetq_lane_u16(__arm_vreinterpretq_u16_u32(y_), 3);
    }

    return crc ^ 0xffffffff;
}

