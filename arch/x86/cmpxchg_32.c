/*
 * Copyright (C) 2009  Pekka Enberg
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */

#include "arch/cmpxchg.h"

#include <stdint.h>

uint64_t cmpxchg_64(uint64_t *p, uint64_t old, uint64_t new)
{
	uint32_t low = new;
	uint32_t high = new >> 32;

	asm volatile (
		"lock; cmpxchg8b %1\n"
		     : "+A" (old), "+m" (*p)
		     :  "b" (low),  "c" (high)
		     );
        return old;
}
