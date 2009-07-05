#ifndef JIT_LIR_PRINTER_H
#define JIT_LIR_PRINTER_H

#include "lib/string.h"

struct insn;

int lir_print(struct insn *, struct string *);

#endif /* JIT_LIR_PRINTER_H */
