#ifndef X86_DECODE_H
#define X86_DECODE_H

#include "arch/isa.h"

#include <stdbool.h>
#include <stdint.h>

static inline bool x86_is_call(unsigned char *insn)
{
	return *insn == 0xe8;
}

static inline unsigned long x86_call_target(unsigned char *insn)
{
	unsigned long start = (unsigned long) insn;
	int32_t relative;

	relative = (uint32_t) insn[1] | (uint32_t) insn[2] << 8 | (uint32_t) insn[3] << 16 | (uint32_t) insn[4] << 24;

	return start + relative + X86_CALL_INSN_SIZE;
}

#endif /* X86_DECODE_H */
