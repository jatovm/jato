#include <stdbool.h>

#include "arch/init.h"

unsigned long x86_cpu_features;

static inline void cpuid(unsigned int *eax, unsigned int *ebx,
			 unsigned int *ecx, unsigned int *edx)
{
	asm("cpuid"
	    : "=a" (*eax),
	      "=b" (*ebx),
	      "=c" (*ecx),
	      "=d" (*edx)
	    : "0" (*eax), "2" (*ecx));
}

static void init_cpu_features(void)
{
	unsigned int eax, ebx, ecx, edx;

	eax = 0x01;
	ecx = 0x00;

	cpuid(&eax, &ebx, &ecx, &edx);

	x86_cpu_features = edx;

}

void arch_init(void)
{
	init_cpu_features();
}
