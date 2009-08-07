#ifndef X86_INIT_H
#define X86_INIT_H 1

#include <stdbool.h>

#define X86_FEATURE_SSE 	25
#define X86_FEATURE_SSE2	26

extern unsigned long x86_cpu_features;

static inline bool cpu_has(unsigned char feature)
{
	return x86_cpu_features & (1UL << feature);
}

void arch_init(void);

#endif /* X86_INIT_H */
