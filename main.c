
#include <stdio.h>
#include <inttypes.h>

#include <arm_mve.h>


// crc32c implementations
struct impl {
    const char *name;
    uint32_t (*crc32c)(uint32_t crc, const void *data, size_t size);
};

extern struct impl impls[];


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
    for (size_t i = 0; impls[i].name; i++) {
        uint32_t crc = impls[i].crc32c(0, data, DATA_SIZE);
        printf("%-42s => 0x%08"PRIx32"%s\n", impls[i].name, crc,
                (crc == 0x9f2076a7) ? "" : " !");
    }
}
