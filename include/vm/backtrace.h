#ifndef __BACKTRACE_H
#define __BACKTRACE_H

#include <signal.h>

extern void print_backtrace_and_die(int, siginfo_t *, void *);
extern void print_trace(void);

#endif
