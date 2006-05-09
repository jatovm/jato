#ifndef __JIT_BYTECODE_CONVERTERS
#define __JIT_BYTECODE_CONVERTERS

#define DECLARE_CONVERTER(name) \
	extern int name(struct compilation_unit *, struct basic_block *, unsigned long);

#include <jit/load-store-bc.h>
#include <jit/arithmetic-bc.h>
#include <jit/typeconv-bc.h>
#include <jit/object-bc.h>
#include <jit/ostack-bc.h>
#include <jit/invoke-bc.h>

#endif
