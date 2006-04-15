#include <execinfo.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

/* get REG_EIP from ucontext.h */
#define __USE_GNU
#include <ucontext.h>

/* Must be inline so this does not change the backtrace for
   bt_sighandler. */
static inline void __print_trace(unsigned long start, void *caller)
{
	void *array[10];
	size_t size;
	char **strings;
	size_t i;

	size = backtrace(array, 10);
	array[1] = caller; 
	strings = backtrace_symbols(array, size);

	printf("Native stack trace:\n");
	for (i = start; i < size; i++)
		printf("  %s\n", strings[i]);

	free(strings);
}

void print_trace(void)
{
	__print_trace(0, NULL);
}

void bt_sighandler(int sig, siginfo_t *info, void *secret)
{
	void *eip;
	ucontext_t *uc = secret;

	eip = (void *) uc->uc_mcontext.gregs[REG_EIP];

	if (sig == SIGSEGV)
		printf("SIGSEGV at EIP %p while accessing memory address %p.\n",
		       eip, info->si_addr);
	else
		printf("Got signal %d\n", sig);

	__print_trace(1, eip);
	exit(1);
}
