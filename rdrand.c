#include "rand.h"

#include <error.h>
#include <stdlib.h>
#include <string.h>

// If rdrand fails, retry this many times
#define RETRY_COUNT 10;

// >1022 calls to 64-bit rdrand is guaranteed to include a reseed.
// https://software.intel.com/en-us/articles/intel-digital-random-number-generator-drng-software-implementation-guide
//   "The DRBG autonomously decides when it needs to be reseeded to refresh the
//   random number pool in the buffer and is both unpredictable and transparent
//   to the RDRAND caller. An upper bound of 511 128-bit samples will be
//   generated per seed. That is, no more than 511*2=1022 sequential DRNG
//   random numbers will be generated from the same seed value."
static const int RANDOM_COUNT = 1024;

// The seed is 256 bits, so that's the amount of actual entropy we're adding
static const int ENTROPY_INCREMENT = 256;

static inline void fill_random_buf(uint64_t *buf)
{
    register unsigned int goodcalls = 0;
    for (int i = 0; i < RANDOM_COUNT; i++)
    {
        register int counter = RETRY_COUNT;
        register uint64_t val asm ("rcx");
        asm volatile ("1:\t"
                      "dec %0\n\t"
                      "je 2f\n\t"
                      ".byte	0x48,0x0f,0xc7,0xf1\n\t"   // rdrand %rcx
                      "jnc 1b\n\t"
                      "2:\t"
                      "adc $0,%1" : "+g" (counter), "+g" (goodcalls) : : "cc");
        buf[i] = val;
    }
    // Keeping a counter and checking later takes a branch out of the inner loop
    if (goodcalls < RANDOM_COUNT) {
        error(EXIT_FAILURE, 0,
              "Error: rdrand failed %d times\n", RANDOM_COUNT - goodcalls);
    }
}

struct entropy get_entropy_rdrand(void)
{
    struct entropy entropy;
    entropy.ent_count = ENTROPY_INCREMENT;
    entropy.size = RANDOM_COUNT * sizeof(uint64_t);
    fill_random_buf(entropy.buf);
    return entropy;
}
