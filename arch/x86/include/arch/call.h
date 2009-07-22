#ifndef __X86_CALL_H
#define __X86_CALL_H

#ifdef CONFIG_X86_32
/**
 * This calls a function with call arguments copied from @args
 * array. The array contains @args_count elements of machine word
 * size. The @target must be a variable holding a function
 * pointer. Call result will be stored in @result.
 */
#define native_call(target, args, args_count, result) {			\
		__asm__ volatile (					\
		  "movl %%ebx, %%ecx \n"				\
		  "shl $2, %%ebx \n"					\
		  "subl %%ebx, %%esp \n"				\
		  "movl %%esp, %%edi \n"				\
		  "cld \n"						\
		  "rep movsd \n"					\
		  "movl %%ebx, %%esi \n"				\
		  "call *%3 \n"						\
		  "addl %%esi, %%esp \n"				\
		  : "=a" (result)					\
		  : "b" (args_count), "S"(args), "m"(target)		\
		  : "%ecx", "%edi", "cc"				\
								); \
	}

/**
 * This calls a VM native function with call arguments copied from
 * @args array. The array contains @args_count elements of machine
 * word size. The @target must be a pointer to a VM function. Call
 * result will be stored in @result.
 */
#define vm_native_call(target, args, args_count, result) {		\
		__asm__ volatile (					\
		  "movl %%ebx, %%ecx \n"				\
		  "shl $2, %%ebx \n"					\
		  "subl %%ebx, %%esp \n"				\
		  "movl %%esp, %%edi \n"				\
		  "cld \n"						\
		  "rep movsd \n"					\
		  "movl %%ebx, %%esi \n"				\
									\
		  "pushl %%esp \n"					\
		  "pushl %3 \n"						\
		  "call vm_enter_vm_native \n"				\
		  "addl $8, %%esp \n"					\
		  "test %%eax, %%eax \n"				\
		  "jnz 1f \n"						\
									\
		  "call * -8(%%esp)\n"					\
		  "movl %%eax, %0 \n"					\
									\
		  "call vm_leave_vm_native \n"				\
									\
		  "1: addl %%esi, %%esp \n"				\
		  : "=r" (result)					\
		  : "b" (args_count), "S"(args), "r"(target)		\
		  : "%ecx", "%edi", "cc"				\
		); \
	}
#else
 #error NOT IMPLEMENTED
#endif

#endif /* __X86_CALL_H */
