#ifndef __BACKTRACE_H
#define __BACKTRACE_H

#include <signal.h>

extern void print_trace(void);
extern void bt_sighandler(int, siginfo_t *, void *);

#endif
