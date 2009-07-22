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
#else
 #error NOT IMPLEMENTED
#endif

#endif /* __X86_CALL_H */
