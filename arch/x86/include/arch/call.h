#ifndef __X86_CALL_H
#define __X86_CALL_H

#ifdef CONFIG_X86_32
/**
 * This calls a function (@target) with call arguments copied from
 * @args array. The array contains @args_count elements of machine
 * word size. The call result will be stored in @result.
 */
#define native_call(target, args, args_count, result) {			\
		__asm__ volatile (					\
		  "movl %2, %%esi \n"					\
		  "movl %1, %%ecx \n"					\
		  "subl %%ecx, %%esp \n"				\
		  "movl %%esp, %%edi \n"				\
		  "cld \n"						\
		  "rep movsb \n"					\
		  "movl %%ecx, %%esi \n"				\
		  "call *%3 \n"						\
		  "addl %%esi, %%esp \n"				\
		  "movl %%eax, %0 \n"					\
		  : "=r" (result)					\
		  : "r" (sizeof(long) * args_count), "r"(args), "m"(target) \
		  : "%ecx", "%esi", "%eax", "cc", "memory" \
								); \
	}
#else
 #error NOT IMPLEMENTED
#endif

#endif /* __X86_CALL_H */
