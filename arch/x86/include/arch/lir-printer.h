#ifndef __JIT_LIR_PRINTER_H
#define __JIT_LIR_PRINTER_H

#include <arch/registers.h>
#include <arch/stack-frame.h>

int lir_print(struct insn *, struct string *);
#endif
