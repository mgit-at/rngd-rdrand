#include "rand.h"

#include <error.h>
#include <stdlib.h>
#include <string.h>

// Number of samples to obtain
#define RANDOM_COUNT 128

// The samples are supposedly independent:
// we get n bits of entropy in an n-bit sample.
static const int ENTROPY_INCREMENT = 64 * RANDOM_COUNT;

static inline void fill_random_buf(uint64_t *buf)
{
    register unsigned int goodcalls = 0;
    for (int i = 0; i < RANDOM_COUNT; i++)
    {
        register int counter = RETRY_COUNT;
        register uint64_t val;
        asm volatile ("1:\t"
                      "dec %1\n\t"
                      "je 2f\n\t"
                      "rdseed %0\n\t"
                      "jnc 1b\n\t"
                      "2:\t"
                      "adc $0,%2" : "=r" (val), "+g" (counter), "+g" (goodcalls) : : "cc");
        buf[i] = val;
    }
    // Keeping a counter and checking later takes a branch out of the inner loop
    if (goodcalls < RANDOM_COUNT) {
        error(EXIT_FAILURE, 0,
              "Error: rdseed failed %d times\n", RANDOM_COUNT - goodcalls);
    }
}

struct entropy get_entropy_rdseed(void)
{
    struct entropy entropy;
    entropy.ent_count = ENTROPY_INCREMENT;
    entropy.size = RANDOM_COUNT * sizeof(uint64_t);
    fill_random_buf(entropy.buf);
    return entropy;
}
