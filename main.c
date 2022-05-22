
#include <stdio.h>
#include <inttypes.h>

#include <arm_mve.h>


// crc32c implementations
extern uint32_t crc32c_naive(uint32_t crc, const void *data, size_t size);
extern uint32_t crc32c_naive_words(uint32_t crc, const void *data, size_t size);
extern uint32_t crc32c_mul(uint32_t crc, const void *data, size_t size);
extern uint32_t crc32c_mul_words(uint32_t crc, const void *data, size_t size);
extern uint32_t crc32c_table(uint32_t crc, const void *data, size_t size);
extern uint32_t crc32c_small_table(uint32_t crc, const void *data, size_t size);
extern uint32_t crc32c_barret_naive(uint32_t crc, const void *data, size_t size);
extern uint32_t crc32c_barret_naive_words(uint32_t crc, const void *data, size_t size);
extern uint32_t crc32c_barret_mul(uint32_t crc, const void *data, size_t size);
extern uint32_t crc32c_barret_mul_words(uint32_t crc, const void *data, size_t size);
extern uint32_t crc32c_barret_sparse(uint32_t crc, const void *data, size_t size);
extern uint32_t crc32c_barret_sparse_words(uint32_t crc, const void *data, size_t size);
extern uint32_t crc32c_barret_vmullp(uint32_t crc, const void *data, size_t size);
extern uint32_t crc32c_barret_vmullp_words(uint32_t crc, const void *data, size_t size);

static inline uint32_t __rbit32(uint32_t a) {
    uint32_t x;
    __asm__(
        "rbit %0,%1"
        : "=r"(x)
        : "r"(a)
    );
    return x;
}

static inline uint16_t __vmullp8(uint8_t a, uint8_t b) {
    uint8x16_t a_ = __arm_vuninitializedq_u8();
    a_ = __arm_vsetq_lane_u8(a, a_, 0);
    uint8x16_t b_ = __arm_vuninitializedq_u8();
    b_ = __arm_vsetq_lane_u8(b, b_, 0);
    uint16x8_t x_ = __arm_vmullbq_poly_p8(a_, b_);
    return __arm_vgetq_lane_u16(x_, 0);
}

static inline uint32_t __vmullp16(uint16_t a, uint16_t b) {
    uint16x8_t a_ = __arm_vuninitializedq_u16();
    a_ = __arm_vsetq_lane_u16(a, a_, 0);
    uint16x8_t b_ = __arm_vuninitializedq_u16();
    b_ = __arm_vsetq_lane_u16(b, b_, 0);
    uint32x4_t x_ = __arm_vmullbq_poly_p16(a_, b_);
    return __arm_vgetq_lane_u32(x_, 0);
}


#define DATA_SIZE 512
uint8_t data[DATA_SIZE];

uint32_t xorshift32(uint32_t *state) {
    uint32_t x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

int main(void) {
    // create some random data
    uint32_t state = 1;
    for (size_t i = 0; i < DATA_SIZE; i++) {
        data[i] = (uint8_t)xorshift32(&state);
    }

    // run crcs
    uint32_t crc;
    crc = crc32c_naive(0, data, DATA_SIZE);
    printf("%-36s => 0x%08"PRIx32"\n", "crc32c_naive", crc);

    crc = crc32c_naive_words(0, data, DATA_SIZE);
    printf("%-36s => 0x%08"PRIx32"\n", "crc32c_naive_words", crc);

    crc = crc32c_mul(0, data, DATA_SIZE);
    printf("%-36s => 0x%08"PRIx32"\n", "crc32c_mul", crc);

    crc = crc32c_mul_words(0, data, DATA_SIZE);
    printf("%-36s => 0x%08"PRIx32"\n", "crc32c_mul_words", crc);

    crc = crc32c_table(0, data, DATA_SIZE);
    printf("%-36s => 0x%08"PRIx32"\n", "crc32c_table", crc);

    crc = crc32c_small_table(0, data, DATA_SIZE);
    printf("%-36s => 0x%08"PRIx32"\n", "crc32c_small_table", crc);

    crc = crc32c_barret_naive(0, data, DATA_SIZE);
    printf("%-36s => 0x%08"PRIx32"\n", "crc32c_barret_naive", crc);

    crc = crc32c_barret_naive_words(0, data, DATA_SIZE);
    printf("%-36s => 0x%08"PRIx32"\n", "crc32c_barret_naive_words", crc);

    crc = crc32c_barret_mul(0, data, DATA_SIZE);
    printf("%-36s => 0x%08"PRIx32"\n", "crc32c_barret_mul", crc);

    crc = crc32c_barret_mul_words(0, data, DATA_SIZE);
    printf("%-36s => 0x%08"PRIx32"\n", "crc32c_barret_mul_words", crc);

    crc = crc32c_barret_sparse(0, data, DATA_SIZE);
    printf("%-36s => 0x%08"PRIx32"\n", "crc32c_barret_sparse", crc);

    crc = crc32c_barret_sparse_words(0, data, DATA_SIZE);
    printf("%-36s => 0x%08"PRIx32"\n", "crc32c_barret_sparse_words", crc);

    crc = crc32c_barret_vmullp(0, data, DATA_SIZE);
    printf("%-36s => 0x%08"PRIx32"\n", "crc32c_barret_vmullp", crc);

    crc = crc32c_barret_vmullp_words(0, data, DATA_SIZE);
    printf("%-36s => 0x%08"PRIx32"\n", "crc32c_barret_vmullp_words", crc);
}
