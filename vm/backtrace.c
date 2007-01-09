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

#ifdef CONFIG_X86_64
#define IP_REG REG_RIP
#define IP_REG_NAME "RIP"
#else
#define IP_REG REG_EIP
#define IP_REG_NAME "EIP"
#endif

static unsigned long get_greg(gregset_t gregs, int reg)
{
	return (unsigned long) gregs[reg];
}

static void show_registers(gregset_t gregs)
{
	unsigned long eax, ebx, ecx, edx;
	unsigned long esi, edi, ebp, esp;
	
	eax = get_greg(gregs, REG_EAX);
	ebx = get_greg(gregs, REG_EBX);
	ecx = get_greg(gregs, REG_ECX);
	edx = get_greg(gregs, REG_EDX);

	esi = get_greg(gregs, REG_ESI);
	edi = get_greg(gregs, REG_EDI);
	ebp = get_greg(gregs, REG_EBP);
	esp = get_greg(gregs, REG_ESP);

	printf("eax: %08lx   ebx: %08lx   ecx: %08lx   edx: %08lx\n",
		eax, ebx, ecx, edx);
	printf("esi: %08lx   edi: %08lx   ebp: %08lx   esp: %08lx\n",
		esi, edi, ebp, esp);
}

void bt_sighandler(int sig, siginfo_t *info, void *secret)
{
	void *eip;
	ucontext_t *uc = secret;

	eip = (void *) uc->uc_mcontext.gregs[IP_REG];

	if (sig == SIGSEGV)
		printf("SIGSEGV at %s %08lx while accessing memory address %08lx.\n",
		       IP_REG_NAME, (unsigned long) eip, (unsigned long) info->si_addr);
	else
		printf("Got signal %d\n", sig);

	show_registers(uc->uc_mcontext.gregs);
	__print_trace(1, eip);
	exit(1);
}
