/*
 * Copyright (C) 2009 Tomek Grabiec <tgrabiec@gmail.com>
 * Copyright (C) 2009 Eduard - Gabriel Munteanu <eduard.munteanu@linux360.ro>
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

#include <stdlib.h>

#include "vm/call.h"
#include "vm/method.h"

#ifdef CONFIG_X86_32

/**
 * This calls a function with call arguments copied from @args
 * array. The array contains @args_count elements of machine word
 * size. The @target must be a variable holding a function
 * pointer. Call result will be stored in @result.
 */
unsigned long native_call(struct vm_method *method,
			  const void *target,
			  unsigned long *args)
{
	unsigned long result;

	__asm__ volatile (
		"movl %%ebx, %%ecx \n"
		"shl $2, %%ebx \n"
		"subl %%ebx, %%esp \n"
		"movl %%esp, %%edi \n"
		"cld \n"
		"rep movsd \n"
		"movl %%ebx, %%esi \n"
		"call *%3 \n"
		"addl %%esi, %%esp \n"
		: "=a" (result)
		: "b" (method->args_count), "S"(args), "m"(target)
		: "%ecx", "%edi", "cc"
	);

	return result;
}

/**
 * This calls a VM native function with call arguments copied from
 * @args array. The array contains @args_count elements of machine
 * word size. The @target must be a pointer to a VM function. Call
 * result will be stored in @result.
 */
unsigned long vm_native_call(struct vm_method *method,
			     const void *target,
			     unsigned long *args)
{
	unsigned long result;

	__asm__ volatile (
		"movl %%ebx, %%ecx \n"
		"shl $2, %%ebx \n"
		"subl %%ebx, %%esp \n"
		"movl %%esp, %%edi \n"
		"cld \n"
		"rep movsd \n"
		"movl %%ebx, %%esi \n"

		"pushl %%esp \n"
		"pushl %3 \n"
		"call vm_enter_vm_native \n"
		"addl $8, %%esp \n"
		"test %%eax, %%eax \n"
		"jnz 1f \n"

		"call * -8(%%esp)\n"
		"movl %%eax, %0 \n"

		"call vm_leave_vm_native \n"

		"1: addl %%esi, %%esp \n"
		: "=r" (result)
		: "b" (method->args_count), "S"(args), "r"(target)
		: "%ecx", "%edi", "cc"
	);

	return result;
}

#else /* CONFIG_X86_32 */

unsigned long native_call(struct vm_method *method,
			  const void *target,
			  unsigned long *args)
{
	abort();

	return 0;
}

unsigned long vm_native_call(struct vm_method *method,
			     const void *target,
			     unsigned long *args)
{
	abort();

	return 0;
}

#warning NOT IMPLEMENTED

#endif /* CONFIG_X86_32 */

