#include <stdbool.h>

#include "arch/init.h"
#include "arch/instruction.h"
#include "vm/die.h"

#include <fpu_control.h>

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

static void setup_fpu(void)
{
	fpu_control_t cw;

	_FPU_GETCW(cw);
	cw		&= ~_FPU_EXTENDED;
	cw		|= _FPU_DOUBLE;
	_FPU_SETCW(cw);
}

void arch_init(void)
{
	insn_sanity_check();

	setup_fpu();

	init_cpu_features();

	if (!cpu_has(X86_FEATURE_SSE))
		warn("CPU does not support SSE. Floating point arithmetic will not work.");
}
