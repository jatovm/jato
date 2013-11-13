/*
 * Copyright (c) 2009 Tomasz Grabiec
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */

#include "arch/stack-frame.h"

#include "vm/natives.h"
#include "vm/method.h"
#include "vm/class.h"

#include "jit/compiler.h"
#include "jit/text.h"

#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>

/* This is located on the first address past the end of the
   uninitialized data segment */
extern char end;

/*
 * Checks whether given address belongs to a native function.
 */
bool is_native(unsigned long eip)
{
	return !is_jit_text((void *)eip);
}

/*
 * Checks whether given address is on heap
 */
bool is_on_heap(unsigned long addr)
{
	return addr >= (unsigned long)&end && addr < (unsigned long)sbrk(0);
}

const char *method_symbol(struct vm_method *method, char *symbol, size_t size)
{
	snprintf(symbol, size, "%s.%s%s", method->class->name, method->name, method->type);

	return symbol;
}
