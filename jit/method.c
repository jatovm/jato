/*
 * Copyright (c) 2009 Tomasz Grabiec
 *
 * This file is released under the GPL version 2 with the following
 * clarification and special exception:
 *
 *     Linking this library statically or dynamically with other modules is
 *     making a combined work based on this library. Thus, the terms and
 *     conditions of the GNU General Public License cover the whole
 *     combination.
 *
 *     As a special exception, the copyright holders of this library give you
 *     permission to link this library with independent modules to produce an
 *     executable, regardless of the license terms of these independent
 *     modules, and to copy and distribute the resulting executable under terms
 *     of your choice, provided that you also meet, for each linked independent
 *     module, the terms and conditions of the license of that module. An
 *     independent module is a module which is not derived from or based on
 *     this library. If you modify this library, you may extend this exception
 *     to your version of the library, but you are not obligated to do so. If
 *     you do not wish to do so, delete this exception statement from your
 *     version.
 *
 * Please refer to the file LICENSE for details.
 */

#include <arch/stack-frame.h>

#include <vm/natives.h>
#include <vm/method.h>
#include <vm/class.h>

#include <jit/compiler.h>
#include <jit/text.h>

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

const char *method_symbol(struct vm_method *method, char *symbol, size_t size)
{
	snprintf(symbol, size, "%s.%s%s", method->class->name, method->name, method->type);

	return symbol;
}
