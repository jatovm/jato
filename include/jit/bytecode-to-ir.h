#ifndef JIT_BYTECODE_TO_IR_H
#define JIT_BYTECODE_TO_IR_H

#include "jit/compiler.h"

/*
 * This macro magic sets up the converter lookup table.
 */

typedef int (*convert_fn_t) (struct parse_context *);

#define DECLARE_CONVERTER(name) int name(struct parse_context *)

#define BYTECODE(opc, name, size, type) DECLARE_CONVERTER(convert_ ## name);
#  include <vm/bytecode-def.h>
#undef BYTECODE


#endif /* JIT_BYTECODE_TO_IR_H */
