/*
 * Copyright (c) 2009 Tomasz Grabiec
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */

#include "arch/thread.h"

static unsigned long get_tls_address(void)
{
	unsigned long result;

#ifdef CONFIG_X86_32
	__asm__(
		"movl %%gs:(0x0), %0 \n"
		: "=r"(result) );
#else
	__asm__(
		"movq %%fs:(0x0), %0 \n"
		: "=r"(result) );
#endif

	return result;
}

unsigned long get_thread_local_offset(void *thread_local_ptr)
{
	return (unsigned long)thread_local_ptr - get_tls_address();
}
