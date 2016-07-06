#include <stdint.h>

#if !defined(__x86_64__)
#error "This only works on 64bit x86 processors!"
#endif

// Size of the entropy buffer
#define BUFFER_SIZE 1024

struct entropy {
    int ent_count;
    int size;
    uint64_t buf[BUFFER_SIZE];
};

struct entropy get_entropy_rdrand(void);
struct entropy get_entropy_rdseed(void);
