#ifndef __JIT_BYTECODE_CONVERTERS
#define __JIT_BYTECODE_CONVERTERS

#define DECLARE_CONVERTER(name) \
	extern int name(struct compilation_unit *, struct basic_block *, unsigned long);

#include <jit/load-store-bc.h>

#endif
