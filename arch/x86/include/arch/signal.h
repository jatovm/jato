#ifndef _ARCH_SIGNAL_H
#define _ARCH_SIGNAL_H

#include <stdbool.h>
#include <stdlib.h>
#include <ucontext.h>

#ifdef CONFIG_X86_32
 #define REG_IP REG_EIP
 #define REG_SP REG_ESP
 #define REG_BP REG_EBP
 #define REG_CX REG_ECX
#elif defined(CONFIG_X86_64)
 #define REG_IP REG_RIP
 #define REG_SP REG_RSP
 #define REG_BP REG_RBP
 #define REG_CX REG_RCX
#else
 #error Unsupported architecture
#endif

struct compilation_unit;

bool signal_from_jit_method(void *ctx);
struct compilation_unit *get_signal_source_cu(void *ctx);

#endif
