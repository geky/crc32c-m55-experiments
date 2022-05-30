// A crc32c implementation using a bitsliced polynomial multiplication
// to fold 128 pairs of 32-bit words (512 bytes) at a time, leveraging
// ARMv8-M's MVE SIMD registers to operate on all 128 bitsliced bits
// simultaneously.

#include <stdint.h>
#include <stddef.h>
#include <string.h>

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
    a_v = (uint16x8_t)__arm_vsetq_lane_u32(a, (uint32x4_t)a_v, 1);
    a_v = (uint16x8_t)__arm_vsetq_lane_u32(a, (uint32x4_t)a_v, 3);
    b_v = (uint16x8_t)__arm_vsetq_lane_u32(b, (uint32x4_t)b_v, 2);
    b_v = (uint16x8_t)__arm_vsetq_lane_u32(b, (uint32x4_t)b_v, 3);

    uint32x4_t x_v = __arm_vmulltq_poly_p16(a_v, b_v);

    return ((uint64_t)__arm_vgetq_lane_u32(x_v, 0))
         ^ ((uint64_t)__arm_vgetq_lane_u32(x_v, 1) << 16)
         ^ ((uint64_t)__arm_vgetq_lane_u32(x_v, 2) << 16)
         ^ ((uint64_t)__arm_vgetq_lane_u32(x_v, 3) << 32);
}

static inline void bitsliced_xorpmul128x64(
        uint32x4_t d[static restrict 64],
        const uint32x4_t a[static restrict 32],
        uint32_t k) {
    for (size_t i = 0; i < 32; i++) {
        if (k & (1 << i)) {
            for (size_t j = 0; j < 32; j++) {
                d[j+i] = __arm_veorq_u32(d[j+i], a[j]);
            }
        }
    }
}

static inline void bitsliced_zero128x64(
        uint32x4_t d[static restrict 64]) {
    memset(d, 0, 64*sizeof(uint32x4_t));
}

static inline uint32_t bitsliced_get32(
        const uint32x4_t d[static restrict 64]) {
    uint32_t r = 0;
    for (size_t j = 0; j < 32; j++) {
        r |= (((const uint32_t*)d)[j*4+3] >> 31) << j;
    }
    return r;
}

static inline void bitsliced_set32(
        uint32x4_t d[static restrict 64],
        uint32_t r) {
    for (size_t j = 0; j < 32; j++) {
         ((uint32_t*)d)[j*4+3] = (r >> j) << 31;
    }
}

static inline void bitsliced_xorload128x64(
        uint32x4_t d[static restrict 64],
        const uint64_t s[static restrict 128]) {
    // note we can't use 8-bit offsets here because our range doesn't
    // actually fit
    // offs_v = [14*8*8 12*8*8 10*8*8... 0*8*8]
    uint16x8_t offs_v = __arm_vshlq_n_u16(__arm_vddupq_n_u16(14, 2), 6);

    // use gathers to transpose lanes, transpose bits in lane manually
    const uint8_t *restrict s_ = (const uint8_t *restrict)s;
    for (size_t j = 0; j < 64; j++) {
        uint8x16_t r = (uint8x16_t)d[j];
        for (size_t i = 0; i < 8; i++) {
            uint8x16_t x = (uint8x16_t)__arm_vsliq_n_u16(
                    __arm_vldrhq_gather_offset_u16(
                        (const uint16_t *restrict)&s_[8*(i+8)+(j/8)], offs_v),
                    __arm_vldrhq_gather_offset_u16(
                        (const uint16_t *restrict)&s_[8*(i+0)+(j/8)], offs_v),
                    8);
            // two-shifts to mask, since there's no single-bit and immediate
            r = __arm_veorq_u8(
                    r,
                    __arm_vshlq_r_u8(
                        __arm_vshrq_n_u8(
                            __arm_vshlq_r_u8(x, 7-(j%8)),
                            7),
                        7-i));
        }
        d[j] = (uint32x4_t)r;
    }
}

static inline void bitsliced_xorshr64(
        uint32x4_t d[static restrict 64],
        uint64_t r) {
    // note we store each bit reversed to take advantage
    // of the vshlc instruction here
    for (size_t j = 0; j < 64; j++) {
        uint32x4_t x = __arm_vdupq_n_u32(((r >> j) << 31));
        uint32x4_t s = __arm_vshlcq_u32(d[j], &(uint32_t){0}, 1);
        d[j] = __arm_veorq_m_u32(s, s, x, 0xf000);
    }
}

static inline void bitsliced_xorshr32(
        uint32x4_t d[static restrict 64],
        uint32_t r) {
    for (size_t j = 0; j < 32; j++) {
        uint32x4_t x = __arm_vdupq_n_u32(((r >> j) << 31));
        uint32x4_t s = d[j+32];
        d[j] = __arm_veorq_m_u32(s, s, x, 0xf000);
    }

    memset(&d[32], 0, 32*sizeof(uint32x4_t));
}


uint32_t crc32c_bitsliced_128x2x32wide(
        uint32_t crc, const void *data, size_t size) {
    const uint8_t *data_ = data;

    // two buffers to alternate between
    uint32x4_t slices[2][64] = {0};
    bool flip = false;

    bitsliced_set32(slices[flip], crc ^ 0xffffffff);

    for (size_t i = 0; i < size;) {
        if (((uintptr_t)&data_[i]) % 1024 == 0 && i+1024+1024 <= size) {
            // xor data into folded
            bitsliced_xorload128x64(slices[flip], (const uint64_t*)&data_[i]);
            // fold with 2 32x32 pmuls, note these already xor
            bitsliced_zero128x64(slices[!flip]);
            bitsliced_xorpmul128x64(slices[!flip],
                    &slices[flip][ 0], 0xfe314258);
            bitsliced_xorpmul128x64(slices[!flip],
                    &slices[flip][32], 0xcdc220dd);
            flip = !flip;
            i += 1024;
        } else if (((uintptr_t)&data_[i]) % 8 == 0 && i+8+8 <= size) {
            uint64_t folded
                    = pmul32(
                        bitsliced_get32(&slices[flip][0])
                            ^ *(const uint32_t*)&data_[i+0],
                        0x493c7d27)
                    ^ pmul32(
                        bitsliced_get32(&slices[flip][32])
                            ^ *(const uint32_t*)&data_[i+4],
                        0xdd45aab8);
            bitsliced_xorshr64(slices[flip], folded);
            i += 8;
        } else if (((uintptr_t)&data_[i]) % 4 == 0 && i+4 <= size) {
            crc = bitsliced_get32(slices[flip])
                    ^ *(const uint32_t *)&data_[i];
            uint32_t b = (uint32_t)pmul32(crc, 0xdea713f1);
            bitsliced_xorshr32(slices[flip],
                    (uint32_t)(pmul32(b, 0x05ec76f1) >> 32)
                    ^ b);
            i += 4;
        } else {
            crc = bitsliced_get32(slices[flip]) ^ data_[i];
            uint32_t b = (uint32_t)pmul32(crc << 24, 0xdea713f1);
            bitsliced_set32(slices[flip], (crc >> 8)
                    ^ (uint32_t)(pmul32(b, 0x05ec76f1) >> 32)
                    ^ b);
            i += 1;
        }
    }

    return bitsliced_get32(slices[flip]) ^ 0xffffffff;
}

