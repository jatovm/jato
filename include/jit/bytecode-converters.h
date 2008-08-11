#ifndef __JIT_BYTECODE_CONVERTERS
#define __JIT_BYTECODE_CONVERTERS

struct parse_context;

#define DECLARE_CONVERTER(name) \
	extern int name(struct parse_context *);

#include <jit/load-store-bc.h>
#include <jit/arithmetic-bc.h>
#include <jit/typeconv-bc.h>
#include <jit/object-bc.h>
#include <jit/ostack-bc.h>
#include <jit/branch-bc.h>
#include <jit/invoke-bc.h>
#include <jit/exception-bc.h>

#endif
