#ifndef JIT_LIR_PRINTER_H
#define JIT_LIR_PRINTER_H

#include <vm/string.h>

struct insn;

int lir_print(struct insn *, struct string *);

#endif /* JIT_LIR_PRINTER_H */
