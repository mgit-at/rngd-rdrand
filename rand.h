#include <stdint.h>

// If rdrand fails, retry this many times
#define RETRY_COUNT 10;

// Size of the entropy buffer
#define BUFFER_SIZE 1024

struct entropy {
    int ent_count;
    int size;
    uint64_t buf[BUFFER_SIZE];
};

struct entropy get_entropy_rdrand(void);
