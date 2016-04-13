#include "cpuid.h"
#include <stdlib.h>

struct cpuid {
  uint32_t eax;
  uint32_t ebx;
  uint32_t ecx;
  uint32_t edx;
};


static inline struct cpuid cpuid(uint i) {
    struct cpuid cpuid;
    asm volatile ("cpuid" : "=a" (cpuid.eax),
                            "=b" (cpuid.ebx),
                            "=c" (cpuid.ecx),
                            "=d" (cpuid.edx)
                          : "a" (i), "c" (0));

    return cpuid;
}

bool has_rdrand(void) {
    struct cpuid cpuid_rdrand = cpuid(1);

    return (cpuid_rdrand.ecx & (1 << 29)) != 0;
}
