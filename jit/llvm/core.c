/*
 * Copyright (c) 2012 Pekka Enberg
 *
 * This file is released under the GPL version 2 with the following
 * clarification and special exception:
 *
 *     Linking this library statically or dynamically with other modules is
 *     making a combined work based on this library. Thus, the terms and
 *     conditions of the GNU General Public License cover the whole
 *     combination.
 *
 *     As a special exception, the copyright holders of this library give you
 *     permission to link this library with independent modules to produce an
 *     executable, regardless of the license terms of these independent
 *     modules, and to copy and distribute the resulting executable under terms
 *     of your choice, provided that you also meet, for each linked independent
 *     module, the terms and conditions of the license of that module. An
 *     independent module is a module which is not derived from or based on
 *     this library. If you modify this library, you may extend this exception
 *     to your version of the library, but you are not obligated to do so. If
 *     you do not wish to do so, delete this exception statement from your
 *     version.
 *
 * Please refer to the file LICENSE for details.
 */

#include "jit/llvm/core.h"

#include "jit/subroutine.h"
#include "lib/stack.h"
#include "vm/method.h"
#include "vm/class.h"

#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/Core.h>

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>

/*
 * Our internal 'compilation unit' and LLVM internal data structures for
 * compiling a method:
 */
struct llvm_context {
	struct compilation_unit		*cu;

	LLVMValueRef			func;

	LLVMBuilderRef			builder;

	/*
	 * Method local variables:
	 */
	LLVMValueRef			*locals;

	/*
	 * A stack that 'mimics' the run-time operand stack during JIT
	 * compilation.
	 */
	struct stack			*mimic_stack;
};

/*
 * Per-process LLVM execution engine and module (is this safe?):
 */
static LLVMExecutionEngineRef	engine;
static LLVMModuleRef		module;

#define LLVM_DEFINE_FUNCTION(name) LLVMValueRef	name ## _func

/*
 * VM internal functions that are called by LLVM generated code:
 */
static LLVM_DEFINE_FUNCTION(vm_object_alloc);
static LLVM_DEFINE_FUNCTION(vm_object_alloc_string_from_utf8);

static inline uint8_t read_u8(unsigned char *code, unsigned long *pos)
{
	uint8_t c = code[*pos];

	*pos = *pos + 1;

	return c;
}

static inline uint16_t read_u16(unsigned char *code, unsigned long *pos)
{
	uint16_t c;

	c  = read_u8(code, pos) << 8;
	c |= read_u8(code, pos);

	return c;
}

/*
 * A helper function that looks like LLVM API for JVM reference types.
 * We treat them as opaque pointers much like 'void *' in C.
 */
static inline LLVMTypeRef LLVMReferenceType(void)
{
	return LLVMPointerType(LLVMInt8Type(), 0);
}

/*
 * A pointer to JVM reference. Used for class and instance field access.
 */
static inline LLVMTypeRef LLVMPointerToReference(void)
{
	return LLVMPointerType(LLVMReferenceType(), 0);
}

static LLVMTypeRef llvm_type(enum vm_type vm_type)
{
	switch (vm_type) {
	case J_VOID:		return LLVMVoidType();
	case J_REFERENCE:	return LLVMReferenceType();
	case J_BYTE:		return LLVMInt8Type();
	case J_SHORT:		return LLVMInt16Type();
	case J_INT:		return LLVMInt32Type();
	case J_LONG:		return LLVMInt64Type();
	case J_CHAR:		return LLVMInt16Type();
	case J_FLOAT:		return LLVMFloatType();
	case J_DOUBLE:		return LLVMDoubleType();
	case J_BOOLEAN:		return LLVMInt8Type();
	default:
		break;
	}

	assert(0);

	return NULL;
}

#define BITS_PER_PTR (sizeof(unsigned long) * 8)

static LLVMValueRef llvm_ptr_to_value(void *p, LLVMTypeRef type)
{
	LLVMValueRef value;

	value	= LLVMConstInt(LLVMIntType(BITS_PER_PTR), (unsigned long) p, 0);

	return LLVMConstIntToPtr(value, type);
}

static LLVMValueRef llvm_lookup_local(struct llvm_context *ctx, unsigned long idx, LLVMTypeRef type)
{
	struct vm_method *vmm = ctx->cu->method;
	LLVMValueRef local;

	if (idx < (unsigned long) vmm->args_count)
		return LLVMGetParam(ctx->func, idx);

	local = ctx->locals[idx];

	if (local)
		return local;

	local = LLVMBuildAlloca(ctx->builder, type, "");

	return ctx->locals[idx] = local;
}

static LLVMTypeRef llvm_function_type(struct vm_method *vmm)
{
	unsigned long ndx = 0, args_count = 0;
	LLVMTypeRef *arg_types = NULL;
	struct vm_method_arg *arg;
	LLVMTypeRef return_type;
	LLVMTypeRef func_type;

	assert(!vm_method_is_jni(vmm));

	args_count = vm_method_arg_stack_count(vmm);

	if (!args_count)
		goto skip_args;

	arg_types = calloc(1, args_count);
	if (!arg_types)
		return NULL;

	if (!vm_method_is_static(vmm))
		arg_types[ndx++] = LLVMReferenceType();

	list_for_each_entry(arg, &vmm->args, list_node) {
		arg_types[ndx++] = llvm_type(arg->type_info.vm_type);
	}

	assert(args_count == ndx);

skip_args:

	return_type = llvm_type(vmm->return_type.vm_type);

	func_type = LLVMFunctionType(return_type, arg_types, args_count, 0);

	free(arg_types);

	return func_type;
}

static void llvm_function_name(struct vm_method *vmm, char *name, size_t len)
{
	char *p;

	snprintf(name, len, "%s.%s%s", vmm->class->name, vmm->name, vmm->type);

	for (p = name; *p; p++)
		if (!isalnum(*p))
			*p = '_';
}

static LLVMValueRef llvm_function(struct llvm_context *ctx)
{
	struct vm_method *vmm = ctx->cu->method;
	LLVMTypeRef func_type;
	char func_name[1024];

	llvm_function_name(vmm, func_name, sizeof(func_name));

	func_type = llvm_function_type(vmm);

	return LLVMAddFunction(module, func_name, func_type);
}

static LLVMValueRef *
llvm_convert_args(struct llvm_context *ctx, struct vm_method *vmm, unsigned long nr_args)
{
	LLVMValueRef *args;
	unsigned long i;

	assert(!vm_method_is_jni(vmm));

	args = calloc(nr_args, sizeof(LLVMValueRef *));
	if (!args)
		return NULL;

	for (i = 0; i < nr_args; i++) {
		args[i++] = stack_pop(ctx->mimic_stack);
	}

	return args;
}

static LLVMValueRef llvm_trampoline(struct vm_method *vmm)
{
	LLVMTypeRef func_type;
	char func_name[1024];
	LLVMValueRef func;

	llvm_function_name(vmm, func_name, sizeof(func_name));

	func = LLVMGetNamedFunction(module, func_name);
	if (func)
		return func;

	func_type	= llvm_function_type(vmm);

	func		= LLVMAddFunction(module, func_name, func_type);

	LLVMAddGlobalMapping(engine, func, vm_method_trampoline_ptr(vmm));

	return func;
}

static int llvm_bc2ir_insn(struct llvm_context *ctx, unsigned char *code, unsigned long *pos)
{
	struct vm_method *vmm = ctx->cu->method;
	unsigned char opc;

	opc = read_u8(code, pos);

	switch (opc) {
	case OPC_NOP: {
		break;
	}
	case OPC_ACONST_NULL: {
		LLVMValueRef value;

		value = LLVMConstNull(LLVMReferenceType());

		stack_push(ctx->mimic_stack, value);

		break;
	}
	case OPC_ICONST_M1:
	case OPC_ICONST_0:
	case OPC_ICONST_1:
	case OPC_ICONST_2:
	case OPC_ICONST_3:
	case OPC_ICONST_4:
	case OPC_ICONST_5: {
		LLVMValueRef value;

		value = LLVMConstInt(LLVMInt32Type(), opc - OPC_ICONST_0, 0);

		stack_push(ctx->mimic_stack, value);

		break;
	}
	case OPC_LCONST_0:
	case OPC_LCONST_1: {
		LLVMValueRef value;

		value = LLVMConstInt(LLVMInt64Type(), opc - OPC_LCONST_0, 0);

		stack_push(ctx->mimic_stack, value);

		break;
	}
	case OPC_FCONST_0:
	case OPC_FCONST_1:
	case OPC_FCONST_2: {
		LLVMValueRef value;

		value = LLVMConstReal(LLVMFloatType(), opc - OPC_FCONST_0);

		stack_push(ctx->mimic_stack, value);

		break;
	}
	case OPC_DCONST_0:
	case OPC_DCONST_1: {
		LLVMValueRef value;

		value = LLVMConstReal(LLVMDoubleType(), opc - OPC_DCONST_0);

		stack_push(ctx->mimic_stack, value);

		break;
	}
	case OPC_BIPUSH: {
		LLVMValueRef value;
		uint8_t i;

		i	= read_u8(code, pos);

		value	= LLVMConstInt(LLVMInt32Type(), i, 0);

		stack_push(ctx->mimic_stack, value);

		break;
	}
	case OPC_SIPUSH: {
		LLVMValueRef value;
		uint16_t i;

		i	= read_u16(code, pos);

		value	= LLVMConstInt(LLVMInt32Type(), i, 0);

		stack_push(ctx->mimic_stack, value);

		break;
	}
	case OPC_LDC: {
		struct vm_method *vmm = ctx->cu->method;
		struct vm_class *vmc = vmm->class;
		struct cafebabe_constant_pool *cp;
		LLVMValueRef value;
		uint8_t cp_idx;

		cp_idx	= read_u8(code, pos);

		if (cafebabe_class_constant_index_invalid(vmc->class, cp_idx)) {
			fprintf(stderr, "abort: OPC_LDC: invalid constant index '%d'!\n", cp_idx);
			assert(0);
		}

		cp = &vmc->class->constant_pool[cp_idx];

		switch (cp->tag) {
		case CAFEBABE_CONSTANT_TAG_INTEGER: {
			int32_t ivalue;

			ivalue	= cafebabe_constant_pool_get_integer(cp);

			value	= LLVMConstInt(LLVMInt32Type(), ivalue, 0);

			break;
		}
		case CAFEBABE_CONSTANT_TAG_LONG: {
			int64_t jvalue;

			jvalue	= cafebabe_constant_pool_get_long(cp);

			value	= LLVMConstInt(LLVMInt64Type(), jvalue, 0);

			break;
		}
		case CAFEBABE_CONSTANT_TAG_FLOAT: {
			float fvalue;

			fvalue	= cafebabe_constant_pool_get_float(cp);

			value	= LLVMConstReal(LLVMFloatType(), fvalue);

			break;
		}
		case CAFEBABE_CONSTANT_TAG_DOUBLE: {
			double dvalue;

			dvalue	= cafebabe_constant_pool_get_double(cp);

			value	= LLVMConstReal(LLVMDoubleType(), dvalue);

			break;
		}
		case CAFEBABE_CONSTANT_TAG_STRING: {
			const struct cafebabe_constant_info_utf8 *utf8;
			LLVMValueRef args[2];

			if (cafebabe_class_constant_get_utf8(vmc->class, cp->string.string_index, &utf8))
				assert(0);

			args[0] = llvm_ptr_to_value(utf8->bytes, LLVMReferenceType());

			args[1] = LLVMConstInt(LLVMInt32Type(), utf8->length, 0);

			value = LLVMBuildCall(ctx->builder, vm_object_alloc_string_from_utf8_func, args, 2, "");

			break;
		}
		case CAFEBABE_CONSTANT_TAG_CLASS: {
			struct vm_class *klass = vm_class_resolve_class(vmc, cp_idx);

			assert(klass != NULL);

			if (vm_class_ensure_object(klass))
				return -1;

			value = llvm_ptr_to_value(klass->object, LLVMReferenceType());

			break;
		}
		default:
			fprintf(stderr, "abort: OPC_LDC: unknown tag '%d'!\n", cp->tag);
			assert(0);
		}

		stack_push(ctx->mimic_stack, value);

		break;
	}
	case OPC_LDC_W:			assert(0); break;
	case OPC_LDC2_W:		assert(0); break;
	case OPC_ILOAD:			assert(0); break;
	case OPC_LLOAD:			assert(0); break;
	case OPC_FLOAD:			assert(0); break;
	case OPC_DLOAD:			assert(0); break;
	case OPC_ALOAD:			assert(0); break;
	case OPC_ILOAD_0:		assert(0); break;
	case OPC_ILOAD_1:		assert(0); break;
	case OPC_ILOAD_2:		assert(0); break;
	case OPC_ILOAD_3:		assert(0); break;
	case OPC_LLOAD_0:		assert(0); break;
	case OPC_LLOAD_1:		assert(0); break;
	case OPC_LLOAD_2:		assert(0); break;
	case OPC_LLOAD_3:		assert(0); break;
	case OPC_FLOAD_0:		assert(0); break;
	case OPC_FLOAD_1:		assert(0); break;
	case OPC_FLOAD_2:		assert(0); break;
	case OPC_FLOAD_3:		assert(0); break;
	case OPC_DLOAD_0:		assert(0); break;
	case OPC_DLOAD_1:		assert(0); break;
	case OPC_DLOAD_2:		assert(0); break;
	case OPC_DLOAD_3:		assert(0); break;
	case OPC_ALOAD_0:
	case OPC_ALOAD_1:
	case OPC_ALOAD_2:
	case OPC_ALOAD_3: {
		LLVMValueRef value;
		uint16_t idx;

		idx = opc - OPC_ALOAD_0;

		value = llvm_lookup_local(ctx, idx, LLVMReferenceType());

		assert(LLVMTypeOf(value) == LLVMReferenceType());

		stack_push(ctx->mimic_stack, value);

		break;
	}
	case OPC_IALOAD:		assert(0); break;
	case OPC_LALOAD:		assert(0); break;
	case OPC_FALOAD:		assert(0); break;
	case OPC_DALOAD:		assert(0); break;
	case OPC_AALOAD:		assert(0); break;
	case OPC_BALOAD:		assert(0); break;
	case OPC_CALOAD:		assert(0); break;
	case OPC_SALOAD:		assert(0); break;
	case OPC_ISTORE:		assert(0); break;
	case OPC_LSTORE:		assert(0); break;
	case OPC_FSTORE:		assert(0); break;
	case OPC_DSTORE:		assert(0); break;
	case OPC_ASTORE:		assert(0); break;
	case OPC_ISTORE_0:		assert(0); break;
	case OPC_ISTORE_1:		assert(0); break;
	case OPC_ISTORE_2:		assert(0); break;
	case OPC_ISTORE_3:		assert(0); break;
	case OPC_LSTORE_0:		assert(0); break;
	case OPC_LSTORE_1:		assert(0); break;
	case OPC_LSTORE_2:		assert(0); break;
	case OPC_LSTORE_3:		assert(0); break;
	case OPC_FSTORE_0:		assert(0); break;
	case OPC_FSTORE_1:		assert(0); break;
	case OPC_FSTORE_2:		assert(0); break;
	case OPC_FSTORE_3:		assert(0); break;
	case OPC_DSTORE_0:		assert(0); break;
	case OPC_DSTORE_1:		assert(0); break;
	case OPC_DSTORE_2:		assert(0); break;
	case OPC_DSTORE_3:		assert(0); break;
	case OPC_ASTORE_0:		assert(0); break;
	case OPC_ASTORE_1:		assert(0); break;
	case OPC_ASTORE_2:		assert(0); break;
	case OPC_ASTORE_3:		assert(0); break;
	case OPC_IASTORE:		assert(0); break;
	case OPC_LASTORE:		assert(0); break;
	case OPC_FASTORE:		assert(0); break;
	case OPC_DASTORE:		assert(0); break;
	case OPC_AASTORE:		assert(0); break;
	case OPC_BASTORE:		assert(0); break;
	case OPC_CASTORE:		assert(0); break;
	case OPC_SASTORE:		assert(0); break;
	case OPC_POP: {
		stack_pop(ctx->mimic_stack);

		break;
	}
	case OPC_POP2:			assert(0); break;
	case OPC_DUP: {
		void *value;

		assert(!stack_is_empty(ctx->mimic_stack));

		value = stack_peek(ctx->mimic_stack);

		stack_push(ctx->mimic_stack, value);

		break;
	}
	case OPC_DUP_X1:		assert(0); break;
	case OPC_DUP_X2:		assert(0); break;
	case OPC_DUP2:			assert(0); break;
	case OPC_DUP2_X1:		assert(0); break;
	case OPC_DUP2_X2:		assert(0); break;
	case OPC_SWAP:			assert(0); break;
	case OPC_IADD:
	case OPC_LADD: {
		LLVMValueRef value1, value2, result;

		value2 = stack_pop(ctx->mimic_stack);

		value1 = stack_pop(ctx->mimic_stack);

		result = LLVMBuildAdd(ctx->builder, value1, value2, "");

		stack_push(ctx->mimic_stack, result);

		break;
	}
	case OPC_FADD:
	case OPC_DADD: {
		LLVMValueRef value1, value2, result;

		value2 = stack_pop(ctx->mimic_stack);

		value1 = stack_pop(ctx->mimic_stack);

		result = LLVMBuildFAdd(ctx->builder, value1, value2, "");

		stack_push(ctx->mimic_stack, result);

		break;
	}
	case OPC_ISUB:
	case OPC_LSUB: {
		LLVMValueRef value1, value2, result;

		value2 = stack_pop(ctx->mimic_stack);

		value1 = stack_pop(ctx->mimic_stack);

		result = LLVMBuildSub(ctx->builder, value1, value2, "");

		stack_push(ctx->mimic_stack, result);

		break;
	}
	case OPC_FSUB:
	case OPC_DSUB: {
		LLVMValueRef value1, value2, result;

		value2 = stack_pop(ctx->mimic_stack);

		value1 = stack_pop(ctx->mimic_stack);

		result = LLVMBuildFSub(ctx->builder, value1, value2, "");

		stack_push(ctx->mimic_stack, result);

		break;
	}
	case OPC_IMUL:
	case OPC_LMUL: {
		LLVMValueRef value1, value2, result;

		value2 = stack_pop(ctx->mimic_stack);

		value1 = stack_pop(ctx->mimic_stack);

		result = LLVMBuildMul(ctx->builder, value1, value2, "");

		stack_push(ctx->mimic_stack, result);

		break;
	}
	case OPC_FMUL:
	case OPC_DMUL: {
		LLVMValueRef value1, value2, result;

		value2 = stack_pop(ctx->mimic_stack);

		value1 = stack_pop(ctx->mimic_stack);

		result = LLVMBuildFMul(ctx->builder, value1, value2, "");

		stack_push(ctx->mimic_stack, result);

		break;
	}
	case OPC_IDIV:
	case OPC_LDIV: {
		LLVMValueRef value1, value2, result;

		value2 = stack_pop(ctx->mimic_stack);

		value1 = stack_pop(ctx->mimic_stack);

		result = LLVMBuildSDiv(ctx->builder, value1, value2, "");

		stack_push(ctx->mimic_stack, result);

		break;
	}
	case OPC_FDIV:
	case OPC_DDIV: {
		LLVMValueRef value1, value2, result;

		value2 = stack_pop(ctx->mimic_stack);

		value1 = stack_pop(ctx->mimic_stack);

		result = LLVMBuildFDiv(ctx->builder, value1, value2, "");

		stack_push(ctx->mimic_stack, result);

		break;
	}
	case OPC_IREM:
	case OPC_LREM: {
		LLVMValueRef value1, value2, result;

		value2 = stack_pop(ctx->mimic_stack);

		value1 = stack_pop(ctx->mimic_stack);

		result = LLVMBuildSRem(ctx->builder, value1, value2, "");

		stack_push(ctx->mimic_stack, result);

		break;
	}
	case OPC_FREM:
	case OPC_DREM: {
		LLVMValueRef value1, value2, result;

		value2 = stack_pop(ctx->mimic_stack);

		value1 = stack_pop(ctx->mimic_stack);

		result = LLVMBuildFRem(ctx->builder, value1, value2, "");

		stack_push(ctx->mimic_stack, result);

		break;
	}
	case OPC_INEG:
	case OPC_LNEG: {
		LLVMValueRef value, result;

		value = stack_pop(ctx->mimic_stack);

		result = LLVMBuildNeg(ctx->builder, value, "");

		stack_push(ctx->mimic_stack, result);

		break;
	}
	case OPC_FNEG:
	case OPC_DNEG: {
		LLVMValueRef value, result;

		value = stack_pop(ctx->mimic_stack);

		result = LLVMBuildFNeg(ctx->builder, value, "");

		stack_push(ctx->mimic_stack, result);

		break;
	}
	case OPC_ISHL:
	case OPC_LSHL: {
		LLVMValueRef value1, value2, result;

		value2 = stack_pop(ctx->mimic_stack);

		value1 = stack_pop(ctx->mimic_stack);

		result = LLVMBuildShl(ctx->builder, value1, value2, "");

		stack_push(ctx->mimic_stack, result);

		break;
	}
	case OPC_ISHR:
	case OPC_LSHR: {
		LLVMValueRef value1, value2, result;

		value2 = stack_pop(ctx->mimic_stack);

		value1 = stack_pop(ctx->mimic_stack);

		result = LLVMBuildLShr(ctx->builder, value1, value2, "");

		stack_push(ctx->mimic_stack, result);

		break;
	}
	case OPC_IUSHR:
	case OPC_LUSHR: {
		LLVMValueRef value1, value2, result;

		value2 = stack_pop(ctx->mimic_stack);

		value1 = stack_pop(ctx->mimic_stack);

		result = LLVMBuildAShr(ctx->builder, value1, value2, "");

		stack_push(ctx->mimic_stack, result);

		break;
	}
	case OPC_IAND:
	case OPC_LAND: {
		LLVMValueRef value1, value2, result;

		value2 = stack_pop(ctx->mimic_stack);

		value1 = stack_pop(ctx->mimic_stack);

		result = LLVMBuildAnd(ctx->builder, value1, value2, "");

		stack_push(ctx->mimic_stack, result);

		break;
	}
	case OPC_IOR:
	case OPC_LOR: {
		LLVMValueRef value1, value2, result;

		value2 = stack_pop(ctx->mimic_stack);

		value1 = stack_pop(ctx->mimic_stack);

		result = LLVMBuildOr(ctx->builder, value1, value2, "");

		stack_push(ctx->mimic_stack, result);

		break;
	}
	case OPC_IXOR:
	case OPC_LXOR: {
		LLVMValueRef value1, value2, result;

		value2 = stack_pop(ctx->mimic_stack);

		value1 = stack_pop(ctx->mimic_stack);

		result = LLVMBuildXor(ctx->builder, value1, value2, "");

		stack_push(ctx->mimic_stack, result);

		break;
	}
	case OPC_IINC:			assert(0); break;
	case OPC_I2L: {
		LLVMValueRef value, result;

		value = stack_pop(ctx->mimic_stack);

		result = LLVMBuildSExt(ctx->builder, value, LLVMInt64Type(), "");

		stack_push(ctx->mimic_stack, result);

		break;
	}
	case OPC_I2F: {
		LLVMValueRef value, result;

		value = stack_pop(ctx->mimic_stack);

		result = LLVMBuildSIToFP(ctx->builder, value, LLVMFloatType(), "");

		stack_push(ctx->mimic_stack, result);

		break;
	}
	case OPC_I2D: {
		LLVMValueRef value, result;

		value = stack_pop(ctx->mimic_stack);

		result = LLVMBuildSIToFP(ctx->builder, value, LLVMDoubleType(), "");

		stack_push(ctx->mimic_stack, result);

		break;
	}
	case OPC_L2I: {
		LLVMValueRef value, result;

		value = stack_pop(ctx->mimic_stack);

		result = LLVMBuildTrunc(ctx->builder, value, LLVMInt32Type(), "");

		stack_push(ctx->mimic_stack, result);

		break;
	}
	case OPC_L2F: {
		LLVMValueRef value, result;

		value = stack_pop(ctx->mimic_stack);

		result = LLVMBuildSIToFP(ctx->builder, value, LLVMFloatType(), "");

		stack_push(ctx->mimic_stack, result);

		break;
	}
	case OPC_L2D: {
		LLVMValueRef value, result;

		value = stack_pop(ctx->mimic_stack);

		result = LLVMBuildSIToFP(ctx->builder, value, LLVMDoubleType(), "");

		stack_push(ctx->mimic_stack, result);

		break;
	}
	case OPC_F2I: {
		LLVMValueRef value, result;

		value = stack_pop(ctx->mimic_stack);

		result = LLVMBuildFPToSI(ctx->builder, value, LLVMInt32Type(), "");

		stack_push(ctx->mimic_stack, result);

		break;
	}
	case OPC_F2L: {
		LLVMValueRef value, result;

		value = stack_pop(ctx->mimic_stack);

		result = LLVMBuildFPToSI(ctx->builder, value, LLVMInt64Type(), "");

		stack_push(ctx->mimic_stack, result);

		break;
	}
	case OPC_F2D: {
		LLVMValueRef value, result;

		value = stack_pop(ctx->mimic_stack);

		result = LLVMBuildFPExt(ctx->builder, value, LLVMDoubleType(), "");

		stack_push(ctx->mimic_stack, result);

		break;
	}
	case OPC_D2I: {
		LLVMValueRef value, result;

		value = stack_pop(ctx->mimic_stack);

		result = LLVMBuildFPToSI(ctx->builder, value, LLVMInt32Type(), "");

		stack_push(ctx->mimic_stack, result);

		break;
	}
	case OPC_D2L: {
		LLVMValueRef value, result;

		value = stack_pop(ctx->mimic_stack);

		result = LLVMBuildFPToSI(ctx->builder, value, LLVMInt64Type(), "");

		stack_push(ctx->mimic_stack, result);

		break;
	}
	case OPC_D2F: {
		LLVMValueRef value, result;

		value = stack_pop(ctx->mimic_stack);

		result = LLVMBuildFPTrunc(ctx->builder, value, LLVMFloatType(), "");

		stack_push(ctx->mimic_stack, result);

		break;
		break;
	}
	case OPC_I2B: {
		LLVMValueRef value, result;

		value = stack_pop(ctx->mimic_stack);

		result = LLVMBuildTrunc(ctx->builder, value, LLVMInt8Type(), "");

		stack_push(ctx->mimic_stack, result);

		break;
	}
	case OPC_I2C: {
		LLVMValueRef value, result;

		value = stack_pop(ctx->mimic_stack);

		result = LLVMBuildTrunc(ctx->builder, value, LLVMInt16Type(), "");

		stack_push(ctx->mimic_stack, result);

		break;
	}
	case OPC_I2S: {
		LLVMValueRef value, result;

		value = stack_pop(ctx->mimic_stack);

		result = LLVMBuildTrunc(ctx->builder, value, LLVMInt16Type(), "");

		stack_push(ctx->mimic_stack, result);

		break;
	}
	case OPC_LCMP:			assert(0); break;
	case OPC_FCMPL:			assert(0); break;
	case OPC_FCMPG:			assert(0); break;
	case OPC_DCMPL:			assert(0); break;
	case OPC_DCMPG:			assert(0); break;
	case OPC_IFEQ:			assert(0); break;
	case OPC_IFNE:			assert(0); break;
	case OPC_IFLT:			assert(0); break;
	case OPC_IFGE:			assert(0); break;
	case OPC_IFGT:			assert(0); break;
	case OPC_IFLE:			assert(0); break;
	case OPC_IF_ICMPEQ:		assert(0); break;
	case OPC_IF_ICMPNE:		assert(0); break;
	case OPC_IF_ICMPLT:		assert(0); break;
	case OPC_IF_ICMPGE:		assert(0); break;
	case OPC_IF_ICMPGT:		assert(0); break;
	case OPC_IF_ICMPLE:		assert(0); break;
	case OPC_IF_ACMPEQ:		assert(0); break;
	case OPC_IF_ACMPNE:		assert(0); break;
	case OPC_GOTO:			assert(0); break;
	case OPC_JSR:			assert(0); break;
	case OPC_RET:			assert(0); break;
	case OPC_TABLESWITCH:		assert(0); break;
	case OPC_LOOKUPSWITCH:		assert(0); break;
	case OPC_IRETURN:
	case OPC_LRETURN:
	case OPC_FRETURN:
	case OPC_DRETURN:
	case OPC_ARETURN: {
		LLVMValueRef value;

		/* XXX: Update monitor state properly before exiting. */
		assert(!method_is_synchronized(vmm));

		value = stack_pop(ctx->mimic_stack);

		LLVMBuildRet(ctx->builder, value);

		stack_clear(ctx->mimic_stack);
		break;
	}
	case OPC_RETURN: {
		/* XXX: Update monitor state properly before exiting. */
		assert(!method_is_synchronized(vmm));

		LLVMBuildRetVoid(ctx->builder);

		stack_clear(ctx->mimic_stack);
		break;
	}
	case OPC_GETSTATIC:		assert(0); break;
	case OPC_PUTSTATIC: {
		struct vm_class *vmc = vmm->class;
		LLVMValueRef target_in;
		struct vm_field *vmf;
		LLVMValueRef value;
		LLVMValueRef addr;
		uint16_t idx;

		idx	= read_u16(code, pos);

		vmf	= vm_class_resolve_field_recursive(vmm->class, idx);

		vm_object_lock(vmc->object);

		/* XXX: Use guard page if necessary */
		assert(vmc->state >= VM_CLASS_INITIALIZING);

		vm_object_unlock(vmc->object);

		target_in = stack_pop(ctx->mimic_stack);

		addr	= llvm_ptr_to_value(vmc->static_values + vmf->offset, LLVMPointerToReference());

		value	= LLVMBuildStore(ctx->builder, target_in, addr);

		stack_push(ctx->mimic_stack, value);

		break;
	}
	case OPC_GETFIELD:		assert(0); break;
	case OPC_PUTFIELD:		assert(0); break;
	case OPC_INVOKEVIRTUAL:		assert(0); break;
	case OPC_INVOKESPECIAL: {
		struct vm_method *target;
		unsigned long nr_args;
		LLVMValueRef value;
		LLVMValueRef *args;
		LLVMValueRef func;
		uint16_t idx;

		idx	= read_u16(code, pos);

		target	= vm_class_resolve_method_recursive(vmm->class, idx, CAFEBABE_CLASS_ACC_STATIC);

		assert(!method_is_synchronized(target));

		nr_args	= vm_method_arg_stack_count(target);

		args	= llvm_convert_args(ctx, target, nr_args);

		func	= llvm_trampoline(target);

		value	= LLVMBuildCall(ctx->builder, func, args, nr_args, "");

		free(args);

		/* XXX: Exception check */

		if (vmm->return_type.vm_type != J_VOID)
			stack_push(ctx->mimic_stack, value);

		break;
	}
	case OPC_INVOKESTATIC:		assert(0); break;
	case OPC_INVOKEINTERFACE:	assert(0); break;
	case OPC_NEW: {
		LLVMValueRef objectref;
		struct vm_class *vmc;
		LLVMValueRef args[1];
		uint16_t idx;

		idx = read_u16(code, pos);

		vmc = vm_class_resolve_class(vmm->class, idx);

		assert(vmc != NULL);

		args[0] = llvm_ptr_to_value(vmc, LLVMReferenceType());

		objectref = LLVMBuildCall(ctx->builder, vm_object_alloc_func, args, 1, "");

		/* XXX: Exception check */

		stack_push(ctx->mimic_stack, objectref);

		break;
	}
	case OPC_NEWARRAY:		assert(0); break;
	case OPC_ANEWARRAY:		assert(0); break;
	case OPC_ARRAYLENGTH:		assert(0); break;
	case OPC_ATHROW:		assert(0); break;
	case OPC_CHECKCAST:		assert(0); break;
	case OPC_INSTANCEOF:		assert(0); break;
	case OPC_MONITORENTER:		assert(0); break;
	case OPC_MONITOREXIT:		assert(0); break;
	case OPC_WIDE:			assert(0); break;
	case OPC_MULTIANEWARRAY:	assert(0); break;
	case OPC_IFNULL:		assert(0); break;
	case OPC_IFNONNULL:		assert(0); break;
	case OPC_GOTO_W:		assert(0); break;
	case OPC_JSR_W:			assert(0); break;
	default:
		fprintf(stderr, "abort: unknown bytecode instruction 0x%02x!\n", opc);
		assert(0);
	}

	return 0;
}

static int llvm_bc2ir_bb(struct llvm_context *ctx, struct basic_block *bb)
{
	struct vm_method *vmm = ctx->cu->method;
	unsigned char *code;
	unsigned long pos;

	if (bb->is_converted)
		return 0;

	code = vmm->code_attribute.code;

	pos = bb->start;

	while (pos < bb->end)
		llvm_bc2ir_insn(ctx, code, &pos);

	bb->is_converted = true;

	return 0;
}

static int llvm_bc2ir(struct llvm_context *ctx)
{
	struct compilation_unit *cu = ctx->cu;
	struct basic_block *bb;
	LLVMBasicBlockRef bbr;

	ctx->builder = LLVMCreateBuilder();

	bbr = LLVMAppendBasicBlock(ctx->func, "L");

	LLVMPositionBuilderAtEnd(ctx->builder, bbr);

	llvm_bc2ir_bb(ctx, cu->entry_bb);

	for_each_basic_block(bb, &cu->bb_list) {
		assert(!bb->is_eh);

#if 0
		bbr = LLVMAppendBasicBlock(ctx->func, "L");

		LLVMPositionBuilderAtEnd(ctx->builder, bbr);
#endif

		llvm_bc2ir_bb(ctx, bb);
	}

	return 0;
}

static int llvm_codegen(struct compilation_unit *cu)
{
	struct vm_method *vmm = cu->method;
	struct llvm_context ctx = {
		.cu = cu,
	};

	ctx.locals = calloc(vmm->code_attribute.max_locals, sizeof(LLVMValueRef));
	if (!ctx.locals)
		return -1;

	ctx.func = llvm_function(&ctx);
	if (!ctx.func)
		return -1;

	ctx.mimic_stack = alloc_stack();

	if (llvm_bc2ir(&ctx) < 0)
		assert(0);

	if (opt_llvm_verbose)
		LLVMDumpValue(ctx.func);

	cu->entry_point = LLVMRecompileAndRelinkFunction(engine, ctx.func);

	assert(stack_is_empty(ctx.mimic_stack));

	free_stack(ctx.mimic_stack);

	free(ctx.locals);

	return 0;
}

int llvm_compile(struct compilation_unit *cu)
{
	struct vm_method *vmm = cu->method;
	struct vm_class *vmc = vmm->class;
	int err;

	if (opt_llvm_verbose)
		fprintf(stderr, "Compiling %s.%s%s...\n", vmc->name, vmm->name, vmm->type);

	err = inline_subroutines(cu->method);
	if (err)
		goto out;

	err = analyze_control_flow(cu);
	if (err)
		goto out;

	err = llvm_codegen(cu);
	if (err)
		goto out;

	LLVMVerifyModule(module, LLVMPrintMessageAction, NULL);

out:
	return err;
}

static LLVMValueRef llvm_setup_func(const char *name, void *func_p, LLVMTypeRef return_type, unsigned args_count, ...)
{
	LLVMTypeRef arg_types[args_count];
	LLVMTypeRef func_type;
	LLVMValueRef func;
	unsigned idx;
	va_list ap;

	va_start(ap, args_count);

	for (idx = 0; idx < args_count; idx++) {
		arg_types[idx]	= va_arg(ap, LLVMTypeRef);
	}

	va_end(ap);

	func_type	= LLVMFunctionType(return_type, arg_types, args_count, 0);

	func		= LLVMAddFunction(module, name, func_type);

	LLVMAddGlobalMapping(engine, func, func_p);

	return func;
}

void llvm_init(void)
{
	LLVMLinkInJIT();

	LLVMInitializeNativeTarget();

	module = LLVMModuleCreateWithName("JIT");
	if (!module)
		assert(0);

	if (LLVMCreateJITCompilerForModule(&engine, module, 2, NULL) < 0)
		assert(0);

	vm_object_alloc_func = llvm_setup_func(
		"vm_object_alloc",
		vm_object_alloc,
		LLVMReferenceType(), 1, LLVMReferenceType());

	vm_object_alloc_string_from_utf8_func = llvm_setup_func(
		"vm_object_alloc_string_from_utf8",
		vm_object_alloc_string_from_utf8,
		LLVMReferenceType(), 2, LLVMReferenceType(), LLVMInt32Type());
}

void llvm_exit(void)
{
	LLVMDisposeExecutionEngine(engine);

	LLVMDisposeModule(module);
}
