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

#include "arch/registers.h"

#include "jit/args.h"

#include "vm/call.h"
#include "vm/method.h"
#include "vm/stack-trace.h"

#ifdef CONFIG_X86_32

static void do_native_call(struct vm_method *method, void *target,
			     unsigned long *args, union jvalue *result)
{
	__asm__ volatile
		(
	 "movl %%ebx, %%ecx \n"
	 "shl $2, %%ebx \n"
	 "subl %%ebx, %%esp \n"
	 "movl %%esp, %%edi \n"
	 "cld \n"
	 "rep movsd \n"
	 "mov %%ebx, %%esi \n"

	 "test %4, %4 \n"
	 "jz 1f \n"

	 "pushl %%esp \n"
	 "pushl %2 \n"
	 "call vm_enter_vm_native \n"
	 "addl $8, %%esp \n"
	 "test %%eax, %%eax \n"
	 "jnz 2f \n"
"1: \n"
	 "call *%2 \n"
	 "movl %3, %%edi \n"
	 "movl %%eax, (%%edi) \n"
	 "movl %%edx, 4(%%edi) \n"
"2: \n"
	 "addl %%esi, %%esp \n"
	 :
	 : "b" (method->args_count),
	   "S" (args),
	   "m" (target),
	   "m" (result),
	   "r" (vm_method_is_vm_native(method))
	 : "%ecx", "%edx", "%edi", "cc", "memory");

	if (vm_method_is_vm_native(method))
		vm_leave_vm_native();
}

/**
 * This calls a function with call arguments copied from @args
 * array. The array contains @args_count elements of machine word
 * size. Call result will be stored in @result.
 */
void native_call(struct vm_method *method, void *target,
		 unsigned long *args, union jvalue *result)
{
	switch (method->return_type.vm_type) {
	case J_VOID: {
		union jvalue unused;

		do_native_call(method, target, args, &unused);
		break;
	}
	case J_REFERENCE:
		do_native_call(method, target, args, result);
		break;
	case J_INT:
		do_native_call(method, target, args, result);
		break;
	case J_CHAR:
		do_native_call(method, target, args, result);
		result->i = (jint) result->c;
		break;
	case J_BYTE:
		do_native_call(method, target, args, result);
		result->i = (jint) result->b;
		break;
	case J_SHORT:
		do_native_call(method, target, args, result);
		result->i = (jint) result->s;
		break;
	case J_BOOLEAN:
		do_native_call(method, target, args, result);
		result->i = (jint) result->z;
		break;
	case J_LONG:
		do_native_call(method, target, args, result);
		break;
	case J_DOUBLE:
	case J_FLOAT:
		error("not implemented");
		break;
	case J_RETURN_ADDRESS:
	case VM_TYPE_MAX:
		die("unexpected type");
	}
}

#else /* CONFIG_X86_32 */

/**
 * Calls @method which address is obtained from a memory
 * pointed by @target. Function returns call result which
 * is supposed to be saved to %rax.
 */
static unsigned long native_call_gp(struct vm_method *method,
				    const void *target,
				    unsigned long *args)
{
	int i;
	size_t sp = 0, r = 0;
	struct vm_args_map *map = method->args_map;
	unsigned long *stack, regs[6];
	unsigned long result;

	stack = malloc(sizeof(unsigned long) * method->args_count);
	if (!stack)
		abort();

	for (i = 0; i < method->args_count; i++) {
		if (map[i].reg == MACH_REG_UNASSIGNED)
			stack[sp++] = args[i];
		else
			regs[r++] = args[i];

		/* Skip duplicate slots. */
		if (map[i].type == J_LONG || map[i].type == J_DOUBLE)
			i++;
	}

	while (r < 6)
		regs[r++] = 0;

	__asm__ volatile (
		/* Copy stack arguments onto the stack. */
		"movq %1, %%rsi \n"
		"movq %2, %%rcx \n"
		"shl $3, %%rcx \n"
		"subq %%rcx, %%rsp \n"
		"movq %%rsp, %%rdi \n"
		"cld \n"
		"rep movsq \n"

		"movq %%rcx, %%r12 \n"

		/* Assign registers to register arguments. */
		"movq 0x00(%4), %%rdi \n"
		"movq 0x08(%4), %%rsi \n"
		"movq 0x10(%4), %%rdx \n"
		"movq 0x18(%4), %%rcx \n"
		"movq 0x20(%4), %%r8 \n"
		"movq 0x28(%4), %%r9 \n"

		"call *%3 \n"
		"addq %%r12, %%rsp \n"
		: "=a" (result)
		: "r" (stack), "r" (sp), "m" (target), "r" (regs)
		: "rdi", "rsi", "rdx", "rcx", "r8", "r9", "r10", "r11", "r12", "cc", "memory"
	);

	free(stack);

	return 0;
}

static unsigned long vm_native_call_gp(struct vm_method *method,
				       const void *target,
				       unsigned long *args)
{
	abort();

	return 0;
}

/**
 * This calls a function with call arguments copied from @args
 * array. The array contains @args_count elements of machine word
 * size. The @target must be a variable holding a function
 * pointer. Call result will be stored in @result.
 */
void native_call(struct vm_method *method,
		 void *target,
		 unsigned long *args,
		 union jvalue *result)
{
	switch (method->return_type.vm_type) {
	case J_VOID:
		native_call_gp(method, target, args);
		break;
	case J_REFERENCE:
		result->l = (jobject) native_call_gp(method, target, args);
		break;
	case J_INT:
		result->i = (jint) native_call_gp(method, target, args);
		break;
	case J_CHAR:
		result->c = (jchar) native_call_gp(method, target, args);
		break;
	case J_BYTE:
		result->b = (jbyte) native_call_gp(method, target, args);
		break;
	case J_SHORT:
		result->s = (jshort) native_call_gp(method, target, args);
		break;
	case J_BOOLEAN:
		result->z = (jboolean) native_call_gp(method, target, args);
		break;
	case J_LONG:
		result->j = (jlong) native_call_gp(method, target, args);
		break;
	case J_DOUBLE:
	case J_FLOAT:
		error("not implemented");
		break;
	case J_RETURN_ADDRESS:
	case VM_TYPE_MAX:
		die("unexpected type");
	}
}

/**
 * This calls a VM native function with call arguments copied from
 * @args array. The array contains @args_count elements of machine
 * word size. The @target must be a pointer to a VM function. Call
 * result will be stored in @result.
 */
void vm_native_call(struct vm_method *method,
		    const void *target,
		    unsigned long *args,
		    union jvalue *result)
{
	switch (method->return_type.vm_type) {
	case J_VOID:
		vm_native_call_gp(method, target, args);
		break;
	case J_REFERENCE:
		result->l = (jobject) vm_native_call_gp(method, target, args);
		break;
	case J_INT:
		result->i = (jint) vm_native_call_gp(method, target, args);
		break;
	case J_CHAR:
		result->c = (jchar) vm_native_call_gp(method, target, args);
		break;
	case J_BYTE:
		result->b = (jbyte) vm_native_call_gp(method, target, args);
		break;
	case J_SHORT:
		result->s = (jshort) vm_native_call_gp(method, target, args);
		break;
	case J_BOOLEAN:
		result->z = (jboolean) vm_native_call_gp(method, target, args);
		break;
	case J_LONG:
		result->j = (jlong) vm_native_call_gp(method, target, args);
		break;
	case J_DOUBLE:
	case J_FLOAT:
		error("not implemented");
		break;
	case J_RETURN_ADDRESS:
	case VM_TYPE_MAX:
		die("unexpected type");
	}
}

#warning NOT IMPLEMENTED

#endif /* CONFIG_X86_32 */

