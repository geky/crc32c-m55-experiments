// A crc32c implementation using a bitsliced polynomial multiplication
// to fold 32 pairs of 32-bit words (512 bytes) at a time

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

static inline void bitsliced_xorpmul32x64(
        uint32_t d[static restrict 64],
        const uint32_t a[static restrict 32],
        uint32_t k) {
    for (size_t i = 0; i < 32; i++) {
        if (k & (1 << i)) {
            for (size_t j = 0; j < 32; j++) {
                d[j+i] ^= a[j];
            }
        }
    }
}

static inline void bitsliced_zero32x64(
        uint32_t d[static restrict 64]) {
    memset(d, 0, 64*sizeof(uint32_t));
}

static inline uint32_t bitsliced_get32(
        const uint32_t d[static restrict 64]) {
    uint32_t r = 0;
    for (size_t j = 0; j < 32; j++) {
        r |= (d[j] & 1) << j;
    }
    return r;
}

static inline void bitsliced_set32(
        uint32_t d[static restrict 64],
        uint32_t r) {
    for (size_t j = 0; j < 32; j++) {
         d[j] = (r >> j) & 1;
    }
}

static inline void bitsliced_xorload32x64(
        uint32_t d[static restrict 64],
        const uint64_t s[static restrict 32]) {
    const uint32_t *restrict s_ = (const uint32_t *restrict)s;
    for (size_t j = 0; j < 64; j++) {
        uint32_t r = d[j];
        for (size_t i = 0; i < 32; i++) {
            r ^= ((s_[2*i+(j/32)] >> (j%32)) & 1) << i;
        }
        d[j] = r;
    }
}

static inline void bitsliced_xorshr64(
        uint32_t d[static restrict 64],
        uint64_t r) {
    for (size_t j = 0; j < 64; j++) {
        d[j] = (d[j] >> 1) ^ ((r >> j) & 1);
    }
}

static inline void bitsliced_xorshr32(
        uint32_t d[static restrict 64],
        uint32_t r) {
    for (size_t j = 0; j < 32; j++) {
        d[j] = d[j+32] ^ ((r >> j) & 1);
    }

    memset(&d[32], 0, 32*sizeof(uint32_t));
}


uint32_t crc32c_bitsliced_32x2x32wide(
        uint32_t crc, const void *data, size_t size) {
    const uint8_t *data_ = data;

    // two buffers to alternate between
    uint32_t slices[2][64] = {{0}, {0}};
    bool flip = false;

    bitsliced_set32(slices[flip], crc ^ 0xffffffff);

    for (size_t i = 0; i < size;) {
        if (((uintptr_t)&data_[i]) % 256 == 0 && i+256+256 <= size) {
            // xor data into folded
            bitsliced_xorload32x64(slices[flip], (const uint64_t*)&data_[i]);
            // fold with 2 32x32 pmuls, note these already xor
            bitsliced_zero32x64(slices[!flip]);
            bitsliced_xorpmul32x64(slices[!flip],
                    &slices[flip][ 0], 0xdcb17aa4);
            bitsliced_xorpmul32x64(slices[!flip],
                    &slices[flip][32], 0x1426a815);
            flip = !flip;
            i += 256;
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

