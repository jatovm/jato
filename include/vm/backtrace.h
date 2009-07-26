#ifndef __BACKTRACE_H
#define __BACKTRACE_H

#include <signal.h>
#include <stdbool.h>

extern void print_backtrace_and_die(int, siginfo_t *, void *);
extern void print_trace(void);
extern void print_trace_from(unsigned long, void*);
extern bool show_exe_function(void *addr);
extern void show_function(void *addr);

#endif
