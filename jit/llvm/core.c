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
#include "jit/compiler.h"	/* for bytecode tracing */
#include "jit/emulate.h"
#include "lib/stack.h"
#include "vm/method.h"
#include "vm/class.h"
#include "vm/trace.h"

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

static void llvm_throw_stub(struct vm_object *exception)
{
	assert(0);
}

/*
 * VM internal functions that are called by LLVM generated code (remember to
 * update `llvm_setup_builtins` if you add new ones):
 */

#define LLVM_DECLARE_BUILTIN(name) LLVMValueRef name ## _func

#define LLVM_DEFINE_BUILTIN(name, return_type, nr_args, ...) 		\
	do {								\
		name ## _func = llvm_setup_func(#name,			\
						name,			\
						return_type,		\
						nr_args,		\
						__VA_ARGS__);		\
	} while (0);

static LLVM_DECLARE_BUILTIN(vm_class_ensure_init);
static LLVM_DECLARE_BUILTIN(vm_object_alloc);
static LLVM_DECLARE_BUILTIN(vm_object_alloc_array);
static LLVM_DECLARE_BUILTIN(vm_object_alloc_primitive_array);
static LLVM_DECLARE_BUILTIN(vm_object_alloc_string_from_utf8);
static LLVM_DECLARE_BUILTIN(vm_object_check_cast);
static LLVM_DECLARE_BUILTIN(vm_object_is_instance_of);
static LLVM_DECLARE_BUILTIN(vm_object_lock);
static LLVM_DECLARE_BUILTIN(vm_object_unlock);
static LLVM_DECLARE_BUILTIN(emulate_dcmpg);
static LLVM_DECLARE_BUILTIN(emulate_dcmpl);
static LLVM_DECLARE_BUILTIN(emulate_fcmpg);
static LLVM_DECLARE_BUILTIN(emulate_fcmpl);
static LLVM_DECLARE_BUILTIN(emulate_lcmp);
static LLVM_DECLARE_BUILTIN(llvm_throw_stub);

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

static inline int16_t read_s16(unsigned char *code, unsigned long *pos)
{
	return read_u16(code, pos);
}

static inline uint32_t read_u32(unsigned char *code, unsigned long *pos)
{
	uint32_t c;

	c  = read_u8(code, pos) << 24;
	c |= read_u8(code, pos) << 16;
	c |= read_u8(code, pos) << 8;
	c |= read_u8(code, pos);

	return c;
}

static inline int32_t read_s32(unsigned char *code, unsigned long *pos)
{
	return read_u32(code, pos);
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
	case J_BYTE:		return LLVMInt32Type();
	case J_SHORT:		return LLVMInt32Type();
	case J_INT:		return LLVMInt32Type();
	case J_LONG:		return LLVMInt64Type();
	case J_CHAR:		return LLVMInt32Type();
	case J_FLOAT:		return LLVMFloatType();
	case J_DOUBLE:		return LLVMDoubleType();
	case J_BOOLEAN:		return LLVMInt32Type();
	default:
		break;
	}

	assert(0);

	return NULL;
}

static bool llvm_is_category_1_type(LLVMTypeRef type)
{
	return type != LLVMInt64Type() && type != LLVMDoubleType();
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
	LLVMBasicBlockRef entry_bbr, current_bbr;
	LLVMValueRef begin;
	LLVMValueRef local;

	local = ctx->locals[idx];

	if (local)
		return local;

	entry_bbr = ctx->cu->entry_bb->priv;

	current_bbr = LLVMGetInsertBlock(ctx->builder);

	begin = LLVMGetFirstInstruction(entry_bbr);

	if (begin)
		LLVMPositionBuilderBefore(ctx->builder, begin);
	else
		LLVMPositionBuilderAtEnd(ctx->builder, entry_bbr);

	local = LLVMBuildAlloca(ctx->builder, type, "");

	LLVMPositionBuilderAtEnd(ctx->builder, current_bbr);

	return ctx->locals[idx] = local;
}

static LLVMValueRef llvm_load_local(struct llvm_context *ctx, unsigned long idx, LLVMTypeRef type)
{
	struct vm_method *vmm = ctx->cu->method;

	if (idx < (unsigned long) vmm->args_count)
		return LLVMGetParam(ctx->func, idx);

	if (!ctx->locals[idx]) {
		LLVMBasicBlockRef entry_bbr, current_bbr;
		LLVMValueRef begin;

		entry_bbr = ctx->cu->entry_bb->priv;

		current_bbr = LLVMGetInsertBlock(ctx->builder);

		begin = LLVMGetFirstInstruction(entry_bbr);

		if (begin)
			LLVMPositionBuilderBefore(ctx->builder, begin);
		else
			LLVMPositionBuilderAtEnd(ctx->builder, entry_bbr);

		ctx->locals[idx] = LLVMBuildAlloca(ctx->builder, type, "");

		LLVMPositionBuilderAtEnd(ctx->builder, current_bbr);
	}

	return LLVMBuildLoad(ctx->builder, ctx->locals[idx], "");
}

static LLVMTypeRef llvm_function_type(struct vm_method *vmm)
{
	unsigned long ndx = 0, args_count = 0;
	LLVMTypeRef *arg_types = NULL;
	struct vm_method_arg *arg;
	LLVMTypeRef return_type;
	LLVMTypeRef func_type;

	assert(!vm_method_is_jni(vmm));

	args_count = count_java_arguments(vmm);

	if (!vm_method_is_static(vmm))
		args_count++;

	if (!args_count)
		goto skip_args;

	arg_types = calloc(args_count, sizeof(LLVMTypeRef));
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

	for (i = 0; i < nr_args; i++)
		args[nr_args - i - 1] = stack_pop(ctx->mimic_stack);

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

static LLVMValueRef llvm_itable_trampoline(struct llvm_context *ctx, struct vm_method *vmm, LLVMValueRef objectref, unsigned long idx)
{
	LLVMValueRef indices[1];
	LLVMTypeRef func_type;
	LLVMValueRef vtable;
	LLVMValueRef klass;
	LLVMValueRef func;
	LLVMValueRef addr;
	LLVMValueRef gep;

	assert(LLVMTypeOf(objectref) == LLVMReferenceType());

	func_type	= llvm_function_type(vmm);

	indices[0] = LLVMConstInt(LLVMInt32Type(), offsetof(struct vm_object, class), 0);

	gep 	= LLVMBuildGEP(ctx->builder, objectref, indices, 1, "");

	addr	= LLVMBuildBitCast(ctx->builder, gep, LLVMPointerType(LLVMReferenceType(), 0), "");

	klass	= LLVMBuildLoad(ctx->builder, addr, "");

	indices[0] = LLVMConstInt(LLVMInt32Type(), offsetof(struct vm_class, itable), 0);

	gep 	= LLVMBuildGEP(ctx->builder, klass, indices, 1, "");

	addr	= LLVMBuildBitCast(ctx->builder, gep, LLVMPointerType(LLVMPointerType(LLVMReferenceType(), 0), 0), "");

	vtable	= LLVMBuildLoad(ctx->builder, addr, "");

	indices[0] = LLVMConstInt(LLVMInt32Type(), idx, 0);

	gep 	= LLVMBuildGEP(ctx->builder, vtable, indices, 1, "");

	addr	= LLVMBuildLoad(ctx->builder, gep, "");

	func	= LLVMBuildBitCast(ctx->builder, addr, LLVMPointerType(func_type, 0), "");

	return func;
}

static LLVMValueRef llvm_vtable_trampoline(struct llvm_context *ctx, struct vm_method *vmm, LLVMValueRef objectref)
{
	LLVMValueRef indices[1];
	LLVMTypeRef func_type;
	LLVMValueRef vtable;
	LLVMValueRef klass;
	LLVMValueRef func;
	LLVMValueRef addr;
	LLVMValueRef gep;

	assert(LLVMTypeOf(objectref) == LLVMReferenceType());

	func_type	= llvm_function_type(vmm);

	indices[0] = LLVMConstInt(LLVMInt32Type(), offsetof(struct vm_object, class), 0);

	gep 	= LLVMBuildGEP(ctx->builder, objectref, indices, 1, "");

	addr	= LLVMBuildBitCast(ctx->builder, gep, LLVMPointerType(LLVMReferenceType(), 0), "");

	klass	= LLVMBuildLoad(ctx->builder, addr, "");

	indices[0] = LLVMConstInt(LLVMInt32Type(), offsetof(struct vm_class, vtable), 0);

	gep 	= LLVMBuildGEP(ctx->builder, klass, indices, 1, "");

	addr	= LLVMBuildBitCast(ctx->builder, gep, LLVMPointerType(LLVMPointerType(LLVMReferenceType(), 0), 0), "");

	vtable	= LLVMBuildLoad(ctx->builder, addr, "");

	indices[0] = LLVMConstInt(LLVMInt32Type(), vmm->virtual_index, 0);

	gep 	= LLVMBuildGEP(ctx->builder, vtable, indices, 1, "");

	addr	= LLVMBuildLoad(ctx->builder, gep, "");

	func	= LLVMBuildBitCast(ctx->builder, addr, LLVMPointerType(func_type, 0), "");

	return func;
}

static LLVMValueRef llvm_build_array_load(struct llvm_context *ctx, LLVMValueRef arrayref, LLVMValueRef index, LLVMTypeRef type)
{
	LLVMValueRef gep, elems, addr;
	LLVMValueRef indices[1];

	indices[0] = LLVMConstInt(LLVMInt32Type(), VM_ARRAY_ELEMS_OFFSET, 0);

	gep 	= LLVMBuildGEP(ctx->builder, arrayref, indices, 1, "");

	elems	= LLVMBuildBitCast(ctx->builder, gep, LLVMPointerType(type, 0), "");

	indices[0] = index;

	addr	= LLVMBuildGEP(ctx->builder, elems, indices, 1, "");

	return LLVMBuildLoad(ctx->builder, addr, "");
}

static void llvm_build_monitorexit(struct llvm_context *ctx, LLVMValueRef objectref)
{
	struct vm_method *vmm;
	LLVMValueRef args[1];

	vmm = ctx->cu->method;

	assert(!vm_method_is_jni(vmm));

	args[0] = objectref;

	LLVMBuildCall(ctx->builder, vm_object_unlock_func, args, 1, "");
}

static void llvm_build_monitorenter(struct llvm_context *ctx, LLVMValueRef objectref)
{
	struct vm_method *vmm;
	LLVMValueRef args[1];

	vmm = ctx->cu->method;

	assert(!vm_method_is_jni(vmm));

	args[0] = objectref;

	LLVMBuildCall(ctx->builder, vm_object_lock_func, args, 1, "");
}

static void llvm_build_ensure_class_init(struct llvm_context *ctx, struct vm_class *vmc)
{
	LLVMValueRef args[1];

	args[0] = llvm_ptr_to_value(vmc, LLVMReferenceType());

	LLVMBuildCall(ctx->builder, vm_class_ensure_init_func, args, 1, "");
}

static void llvm_build_if(struct llvm_context *ctx,
			  unsigned long insn_start,
			  unsigned char *code,
			  unsigned long *pos,
			  LLVMIntPredicate op,
			  LLVMValueRef rhs,
			  LLVMValueRef lhs)
{
	struct basic_block *then_bb;
	struct basic_block *else_bb;
	LLVMValueRef cmp;
	int16_t offset;

	offset = read_s16(code, pos);

	then_bb	= find_bb(ctx->cu, insn_start + offset);

	else_bb	= find_bb(ctx->cu, *pos);

	cmp	= LLVMBuildICmp(ctx->builder, op, rhs, lhs, "");

	LLVMBuildCondBr(ctx->builder, cmp, then_bb->priv, else_bb->priv);
}

static int llvm_bc2ir_insn(struct llvm_context *ctx, unsigned char *code, unsigned long *pos)
{
	struct vm_method *vmm = ctx->cu->method;
	unsigned long insn_start;
	unsigned char opc;
	bool wide = false;

	insn_start = *pos;

restart:
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
	case OPC_ILOAD: {
		LLVMValueRef value;
		uint16_t idx;

		if (wide)
			idx = read_u16(code, pos);
		else
			idx = read_u8(code, pos);

		value = llvm_load_local(ctx, idx, LLVMInt32Type());

		assert(LLVMTypeOf(value) == LLVMInt32Type());

		stack_push(ctx->mimic_stack, value);

		break;
	}
	case OPC_LLOAD: {
		LLVMValueRef value;
		uint16_t idx;

		if (wide)
			idx = read_u16(code, pos);
		else
			idx = read_u8(code, pos);

		value = llvm_load_local(ctx, idx, LLVMInt64Type());

		assert(LLVMTypeOf(value) == LLVMInt64Type());

		stack_push(ctx->mimic_stack, value);

		break;
	}
	case OPC_FLOAD: {
		LLVMValueRef value;
		uint16_t idx;

		if (wide)
			idx = read_u16(code, pos);
		else
			idx = read_u8(code, pos);

		value = llvm_load_local(ctx, idx, LLVMFloatType());

		assert(LLVMTypeOf(value) == LLVMFloatType());

		stack_push(ctx->mimic_stack, value);

		break;
	}
	case OPC_DLOAD: {
		LLVMValueRef value;
		uint16_t idx;

		if (wide)
			idx = read_u16(code, pos);
		else
			idx = read_u8(code, pos);

		value = llvm_load_local(ctx, idx, LLVMDoubleType());

		assert(LLVMTypeOf(value) == LLVMDoubleType());

		stack_push(ctx->mimic_stack, value);

		break;
	}
	case OPC_ALOAD: {
		LLVMValueRef value;
		uint16_t idx;

		if (wide)
			idx = read_u16(code, pos);
		else
			idx = read_u8(code, pos);

		value = llvm_load_local(ctx, idx, LLVMReferenceType());

		assert(LLVMTypeOf(value) == LLVMReferenceType());

		stack_push(ctx->mimic_stack, value);

		break;
	}
	case OPC_ILOAD_0:
	case OPC_ILOAD_1:
	case OPC_ILOAD_2:
	case OPC_ILOAD_3: {
		LLVMValueRef value;
		uint16_t idx;

		idx = opc - OPC_ILOAD_0;

		value = llvm_load_local(ctx, idx, LLVMInt32Type());

		assert(LLVMTypeOf(value) == LLVMInt32Type());

		stack_push(ctx->mimic_stack, value);

		break;
	}
	case OPC_LLOAD_0:
	case OPC_LLOAD_1:
	case OPC_LLOAD_2:
	case OPC_LLOAD_3: {
		LLVMValueRef value;
		uint16_t idx;

		idx = opc - OPC_LLOAD_0;

		value = llvm_load_local(ctx, idx, LLVMInt64Type());

		assert(LLVMTypeOf(value) == LLVMInt64Type());

		stack_push(ctx->mimic_stack, value);

		break;
	}
	case OPC_FLOAD_0:
	case OPC_FLOAD_1:
	case OPC_FLOAD_2:
	case OPC_FLOAD_3: {
		LLVMValueRef value;
		uint16_t idx;

		idx = opc - OPC_FLOAD_0;

		value = llvm_load_local(ctx, idx, LLVMFloatType());

		assert(LLVMTypeOf(value) == LLVMFloatType());

		stack_push(ctx->mimic_stack, value);

		break;
	}
	case OPC_DLOAD_0:
	case OPC_DLOAD_1:
	case OPC_DLOAD_2:
	case OPC_DLOAD_3: {
		LLVMValueRef value;
		uint16_t idx;

		idx = opc - OPC_DLOAD_0;

		value = llvm_load_local(ctx, idx, LLVMDoubleType());

		assert(LLVMTypeOf(value) == LLVMDoubleType());

		stack_push(ctx->mimic_stack, value);

		break;
	}
	case OPC_ALOAD_0:
	case OPC_ALOAD_1:
	case OPC_ALOAD_2:
	case OPC_ALOAD_3: {
		LLVMValueRef value;
		uint16_t idx;

		idx = opc - OPC_ALOAD_0;

		value = llvm_load_local(ctx, idx, LLVMReferenceType());

		assert(LLVMTypeOf(value) == LLVMReferenceType());

		stack_push(ctx->mimic_stack, value);

		break;
	}
	case OPC_IALOAD: {
		LLVMValueRef arrayref, index, value;

		index		= stack_pop(ctx->mimic_stack);
		arrayref	= stack_pop(ctx->mimic_stack);

		value = llvm_build_array_load(ctx, arrayref, index, llvm_type(J_INT));

		stack_push(ctx->mimic_stack, value);

		break;
	}
	case OPC_LALOAD: {
		LLVMValueRef arrayref, index, value;

		index		= stack_pop(ctx->mimic_stack);
		arrayref	= stack_pop(ctx->mimic_stack);

		value = llvm_build_array_load(ctx, arrayref, index, llvm_type(J_LONG));

		stack_push(ctx->mimic_stack, value);

		break;
	}
	case OPC_FALOAD: {
		LLVMValueRef arrayref, index, value;

		index		= stack_pop(ctx->mimic_stack);
		arrayref	= stack_pop(ctx->mimic_stack);

		value = llvm_build_array_load(ctx, arrayref, index, llvm_type(J_FLOAT));

		stack_push(ctx->mimic_stack, value);

		break;
	}
	case OPC_DALOAD: {
		LLVMValueRef arrayref, index, value;

		index		= stack_pop(ctx->mimic_stack);
		arrayref	= stack_pop(ctx->mimic_stack);

		value = llvm_build_array_load(ctx, arrayref, index, llvm_type(J_DOUBLE));

		stack_push(ctx->mimic_stack, value);

		break;
	}
	case OPC_AALOAD: {
		LLVMValueRef arrayref, index, value;

		index		= stack_pop(ctx->mimic_stack);
		arrayref	= stack_pop(ctx->mimic_stack);

		value = llvm_build_array_load(ctx, arrayref, index, llvm_type(J_REFERENCE));

		stack_push(ctx->mimic_stack, value);

		break;
	}
	case OPC_BALOAD: {
		LLVMValueRef arrayref, index, value;

		index		= stack_pop(ctx->mimic_stack);
		arrayref	= stack_pop(ctx->mimic_stack);

		value = llvm_build_array_load(ctx, arrayref, index, llvm_type(J_BYTE));

		stack_push(ctx->mimic_stack, value);

		break;
	}
	case OPC_CALOAD: {
		LLVMValueRef arrayref, index, value;

		index		= stack_pop(ctx->mimic_stack);
		arrayref	= stack_pop(ctx->mimic_stack);

		value = llvm_build_array_load(ctx, arrayref, index, llvm_type(J_CHAR));

		stack_push(ctx->mimic_stack, value);

		break;
	}
	case OPC_SALOAD: {
		LLVMValueRef arrayref, index, value;

		index		= stack_pop(ctx->mimic_stack);
		arrayref	= stack_pop(ctx->mimic_stack);

		value = llvm_build_array_load(ctx, arrayref, index, llvm_type(J_SHORT));

		stack_push(ctx->mimic_stack, value);

		break;
	}
	case OPC_ISTORE: {
		LLVMValueRef value, addr;
		uint16_t idx;

		if (wide)
			idx = read_u16(code, pos);
		else
			idx = read_u8(code, pos);

		addr	= llvm_lookup_local(ctx, idx, LLVMInt32Type());

		value	= stack_pop(ctx->mimic_stack);

		LLVMBuildStore(ctx->builder, value, addr);

		break;
	}
	case OPC_LSTORE: {
		LLVMValueRef value, addr;
		uint16_t idx;

		if (wide)
			idx = read_u16(code, pos);
		else
			idx = read_u8(code, pos);

		addr	= llvm_lookup_local(ctx, idx, LLVMInt64Type());

		value	= stack_pop(ctx->mimic_stack);

		LLVMBuildStore(ctx->builder, value, addr);

		break;
	}
	case OPC_FSTORE: {
		LLVMValueRef value, addr;
		uint16_t idx;

		if (wide)
			idx = read_u16(code, pos);
		else
			idx = read_u8(code, pos);

		addr	= llvm_lookup_local(ctx, idx, LLVMFloatType());

		value	= stack_pop(ctx->mimic_stack);

		LLVMBuildStore(ctx->builder, value, addr);

		break;
	}
	case OPC_DSTORE: {
		LLVMValueRef value, addr;
		uint16_t idx;

		if (wide)
			idx = read_u16(code, pos);
		else
			idx = read_u8(code, pos);

		addr	= llvm_lookup_local(ctx, idx, LLVMDoubleType());

		value	= stack_pop(ctx->mimic_stack);

		LLVMBuildStore(ctx->builder, value, addr);

		break;
	}
	case OPC_ASTORE: {
		LLVMValueRef value, addr;
		uint16_t idx;

		if (wide)
			idx = read_u16(code, pos);
		else
			idx = read_u8(code, pos);

		addr	= llvm_lookup_local(ctx, idx, LLVMReferenceType());

		value	= stack_pop(ctx->mimic_stack);

		LLVMBuildStore(ctx->builder, value, addr);

		break;
	}
	case OPC_ISTORE_0:
	case OPC_ISTORE_1:
	case OPC_ISTORE_2:
	case OPC_ISTORE_3: {
		LLVMValueRef value, addr;
		uint16_t idx;

		idx	= opc - OPC_ISTORE_0;

		addr	= llvm_lookup_local(ctx, idx, LLVMInt32Type());

		value	= stack_pop(ctx->mimic_stack);

		LLVMBuildStore(ctx->builder, value, addr);

		break;
	}
	case OPC_LSTORE_0:
	case OPC_LSTORE_1:
	case OPC_LSTORE_2:
	case OPC_LSTORE_3: {
		LLVMValueRef value, addr;
		uint16_t idx;

		idx	= opc - OPC_LSTORE_0;

		addr	= llvm_lookup_local(ctx, idx, LLVMInt64Type());

		value	= stack_pop(ctx->mimic_stack);

		LLVMBuildStore(ctx->builder, value, addr);

		break;
	}
	case OPC_FSTORE_0:
	case OPC_FSTORE_1:
	case OPC_FSTORE_2:
	case OPC_FSTORE_3: {
		LLVMValueRef value, addr;
		uint16_t idx;

		idx	= opc - OPC_FSTORE_0;

		addr	= llvm_lookup_local(ctx, idx, LLVMFloatType());

		value	= stack_pop(ctx->mimic_stack);

		LLVMBuildStore(ctx->builder, value, addr);

		break;
	}
	case OPC_DSTORE_0:
	case OPC_DSTORE_1:
	case OPC_DSTORE_2:
	case OPC_DSTORE_3: {
		LLVMValueRef value, addr;
		uint16_t idx;

		idx	= opc - OPC_DSTORE_0;

		addr	= llvm_lookup_local(ctx, idx, LLVMDoubleType());

		value	= stack_pop(ctx->mimic_stack);

		LLVMBuildStore(ctx->builder, value, addr);

		break;
	}
	case OPC_ASTORE_0:
	case OPC_ASTORE_1:
	case OPC_ASTORE_2:
	case OPC_ASTORE_3: {
		LLVMValueRef value, addr;
		uint16_t idx;

		idx	= opc - OPC_ASTORE_0;

		addr	= llvm_lookup_local(ctx, idx, LLVMReferenceType());

		value	= stack_pop(ctx->mimic_stack);

		LLVMBuildStore(ctx->builder, value, addr);

		break;
	}
	case OPC_IASTORE:
	case OPC_LASTORE:
	case OPC_FASTORE:
	case OPC_DASTORE:
	case OPC_AASTORE:
	case OPC_BASTORE:
	case OPC_CASTORE:
	case OPC_SASTORE: {
		LLVMValueRef arrayref, index, value;
		LLVMValueRef gep, elems, addr;
		LLVMValueRef indices[1];

		value		= stack_pop(ctx->mimic_stack);
		index		= stack_pop(ctx->mimic_stack);
		arrayref	= stack_pop(ctx->mimic_stack);

		indices[0] = LLVMConstInt(LLVMInt32Type(), VM_ARRAY_ELEMS_OFFSET, 0);

		gep 	= LLVMBuildGEP(ctx->builder, arrayref, indices, 1, "");

		elems	= LLVMBuildBitCast(ctx->builder, gep, LLVMPointerType(LLVMTypeOf(value), 0), "");

		indices[0] = index;

		addr	= LLVMBuildGEP(ctx->builder, elems, indices, 1, "");

		LLVMBuildStore(ctx->builder, value, addr);

		break;
	}
	case OPC_POP: {
		stack_pop(ctx->mimic_stack);

		break;
	}
	case OPC_POP2: {
		LLVMValueRef value;

		value = stack_pop(ctx->mimic_stack);

		if (llvm_is_category_1_type(LLVMTypeOf(value)))
			stack_pop(ctx->mimic_stack);

		break;
	}
	case OPC_DUP: {
		void *value;

		value = stack_peek(ctx->mimic_stack);

		assert(llvm_is_category_1_type(LLVMTypeOf(value)));

		stack_push(ctx->mimic_stack, value);

		break;
	}
	case OPC_DUP_X1: {
		void *value1, *value2;

		value1 = stack_pop(ctx->mimic_stack);
		value2 = stack_pop(ctx->mimic_stack);

		assert(llvm_is_category_1_type(LLVMTypeOf(value1)));
		assert(llvm_is_category_1_type(LLVMTypeOf(value2)));

		stack_push(ctx->mimic_stack, value1);
		stack_push(ctx->mimic_stack, value2);
		stack_push(ctx->mimic_stack, value1);

		break;
	}
	case OPC_DUP_X2: {
		void *value1, *value2;

		value1 = stack_pop(ctx->mimic_stack);

		assert(llvm_is_category_1_type(LLVMTypeOf(value1)));

		value2 = stack_pop(ctx->mimic_stack);

		if (llvm_is_category_1_type(LLVMTypeOf(value2))) {
			/* Form 1 */
			void *value3;

			value3 = stack_pop(ctx->mimic_stack);

			assert(llvm_is_category_1_type(LLVMTypeOf(value3)));

			stack_push(ctx->mimic_stack, value1);
			stack_push(ctx->mimic_stack, value3);
		} else {
			/* Form 2 */
			stack_push(ctx->mimic_stack, value1);
		}

		stack_push(ctx->mimic_stack, value2);
		stack_push(ctx->mimic_stack, value1);

		break;
	}
	case OPC_DUP2: {
		void *value1;

		value1 = stack_pop(ctx->mimic_stack);

		if (llvm_is_category_1_type(LLVMTypeOf(value1))) {
			/* Form 1 */
			void *value2;

			value2 = stack_pop(ctx->mimic_stack);

			assert(llvm_is_category_1_type(LLVMTypeOf(value2)));

			stack_push(ctx->mimic_stack, value1);
			stack_push(ctx->mimic_stack, value2);
		} else {
			/* Form 2 */
			stack_push(ctx->mimic_stack, value1);
		}

		stack_push(ctx->mimic_stack, value1);

		break;
	}
	case OPC_DUP2_X1: {
		void *value1, *value2;

		value1 = stack_pop(ctx->mimic_stack);
		value2 = stack_pop(ctx->mimic_stack);

		assert(llvm_is_category_1_type(LLVMTypeOf(value2)));

		if (llvm_is_category_1_type(LLVMTypeOf(value1))) {
			/* Form 1 */
			void *value3;

			value3 = stack_pop(ctx->mimic_stack);

			assert(llvm_is_category_1_type(LLVMTypeOf(value3)));

			stack_push(ctx->mimic_stack, value2);
			stack_push(ctx->mimic_stack, value1);
			stack_push(ctx->mimic_stack, value3);
		} else {
			/* Form 2 */
			stack_push(ctx->mimic_stack, value1);
		}

		stack_push(ctx->mimic_stack, value2);
		stack_push(ctx->mimic_stack, value1);

		break;
	}
	case OPC_DUP2_X2: {
		void *value1, *value2;

		value1 = stack_pop(ctx->mimic_stack);
		value2 = stack_pop(ctx->mimic_stack);

		if (llvm_is_category_1_type(LLVMTypeOf(value1))) {
			void *value3;

			assert(llvm_is_category_1_type(LLVMTypeOf(value2)));

			value3 = stack_pop(ctx->mimic_stack);

			if (llvm_is_category_1_type(LLVMTypeOf(value3))) {
				/* Form 1 */
				void *value4;

				value4 = stack_pop(ctx->mimic_stack);

				assert(llvm_is_category_1_type(LLVMTypeOf(value4)));

				stack_push(ctx->mimic_stack, value2);
				stack_push(ctx->mimic_stack, value1);
				stack_push(ctx->mimic_stack, value4);
				stack_push(ctx->mimic_stack, value3);
			} else {
				/* Form 3 */
				stack_push(ctx->mimic_stack, value2);
				stack_push(ctx->mimic_stack, value1);
				stack_push(ctx->mimic_stack, value3);
			}
		} else {
			if (llvm_is_category_1_type(LLVMTypeOf(value2))) {
				/* Form 2 */
				void *value3;

				value3 = stack_pop(ctx->mimic_stack);

				assert(llvm_is_category_1_type(LLVMTypeOf(value3)));

				stack_push(ctx->mimic_stack, value1);
				stack_push(ctx->mimic_stack, value3);
			} else {
				/* Form 4 */
				stack_push(ctx->mimic_stack, value1);
			}
		}

		stack_push(ctx->mimic_stack, value2);
		stack_push(ctx->mimic_stack, value1);

		break;
	}
	case OPC_SWAP: {
		LLVMValueRef value1, value2;

		value1 = stack_pop(ctx->mimic_stack);

		value2 = stack_pop(ctx->mimic_stack);

		stack_push(ctx->mimic_stack, value1);

		stack_push(ctx->mimic_stack, value2);

		break;
	}
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
	case OPC_IINC: {
		LLVMValueRef local, addr, value, result;
		uint16_t idx;
		int16_t i;

		if (wide) {
			idx	= read_u16(code, pos);
			i	= read_u16(code, pos);
		} else {
			idx	= read_u8(code, pos);
			i	= read_u8(code, pos);
		}

		local	= llvm_load_local(ctx, idx, LLVMInt32Type());

		addr	= llvm_lookup_local(ctx, idx, LLVMInt32Type());

		value	= LLVMConstInt(LLVMInt32Type(), i, 0);

		result	= LLVMBuildAdd(ctx->builder, local, value, "");

		LLVMBuildStore(ctx->builder, result, addr);

		break;
	}
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
	}
	case OPC_I2B: {
		LLVMValueRef value, result, tmp;

		value	= stack_pop(ctx->mimic_stack);

		tmp	= LLVMBuildShl(ctx->builder, value, LLVMConstInt(LLVMInt32Type(), 24, 0), "");

		result	= LLVMBuildAShr(ctx->builder, tmp, LLVMConstInt(LLVMInt32Type(), 24, 0), "");

		stack_push(ctx->mimic_stack, result);

		break;
	}
	case OPC_I2C: {
		LLVMValueRef value, result;

		value	= stack_pop(ctx->mimic_stack);

		result	= LLVMBuildAnd(ctx->builder, value, LLVMConstInt(LLVMInt32Type(), 65535, 0), "");

		stack_push(ctx->mimic_stack, result);

		break;
	}
	case OPC_I2S: {
		LLVMValueRef value, result, tmp;

		value	= stack_pop(ctx->mimic_stack);

		tmp	= LLVMBuildShl(ctx->builder, value, LLVMConstInt(LLVMInt32Type(), 16, 0), "");

		result	= LLVMBuildAShr(ctx->builder, tmp, LLVMConstInt(LLVMInt32Type(), 16, 0), "");

		stack_push(ctx->mimic_stack, result);

		break;
	}
	case OPC_LCMP: {
		LLVMValueRef values[2];
		LLVMValueRef result;

		values[1] = stack_pop(ctx->mimic_stack);
		values[0] = stack_pop(ctx->mimic_stack);

		result = LLVMBuildCall(ctx->builder, emulate_lcmp_func, values, 2, "");

		stack_push(ctx->mimic_stack, result);

		break;
	}
	case OPC_FCMPL: {
		LLVMValueRef values[2];
		LLVMValueRef result;

		values[1] = stack_pop(ctx->mimic_stack);
		values[0] = stack_pop(ctx->mimic_stack);

		result = LLVMBuildCall(ctx->builder, emulate_fcmpl_func, values, 2, "");

		stack_push(ctx->mimic_stack, result);

		break;
	}
	case OPC_FCMPG: {
		LLVMValueRef values[2];
		LLVMValueRef result;

		values[1] = stack_pop(ctx->mimic_stack);
		values[0] = stack_pop(ctx->mimic_stack);

		result = LLVMBuildCall(ctx->builder, emulate_fcmpg_func, values, 2, "");

		stack_push(ctx->mimic_stack, result);

		break;
	}
	case OPC_DCMPL: {
		LLVMValueRef values[2];
		LLVMValueRef result;

		values[1] = stack_pop(ctx->mimic_stack);
		values[0] = stack_pop(ctx->mimic_stack);

		result = LLVMBuildCall(ctx->builder, emulate_dcmpl_func, values, 2, "");

		stack_push(ctx->mimic_stack, result);

		break;
	}
	case OPC_DCMPG: {
		LLVMValueRef values[2];
		LLVMValueRef result;

		values[1] = stack_pop(ctx->mimic_stack);
		values[0] = stack_pop(ctx->mimic_stack);

		result = LLVMBuildCall(ctx->builder, emulate_dcmpg_func, values, 2, "");

		stack_push(ctx->mimic_stack, result);

		break;
	}
	case OPC_IFEQ: {
		LLVMValueRef value, zero;

		value	= stack_pop(ctx->mimic_stack);

		zero	= LLVMConstInt(LLVMTypeOf(value), 0, 0);

		llvm_build_if(ctx, insn_start, code, pos, LLVMIntEQ, value, zero);

		break;
	}
	case OPC_IFNE: {
		LLVMValueRef value, zero;

		value	= stack_pop(ctx->mimic_stack);

		zero	= LLVMConstInt(LLVMTypeOf(value), 0, 0);

		llvm_build_if(ctx, insn_start, code, pos, LLVMIntNE, value, zero);

		break;
	}
	case OPC_IFLT: {
		LLVMValueRef value, zero;

		value	= stack_pop(ctx->mimic_stack);

		zero	= LLVMConstInt(LLVMTypeOf(value), 0, 0);

		llvm_build_if(ctx, insn_start, code, pos, LLVMIntSLT, value, zero);

		break;
	}
	case OPC_IFGE: {
		LLVMValueRef value, zero;

		value	= stack_pop(ctx->mimic_stack);

		zero	= LLVMConstInt(LLVMTypeOf(value), 0, 0);

		llvm_build_if(ctx, insn_start, code, pos, LLVMIntSGE, value, zero);

		break;
	}
	case OPC_IFGT: {
		LLVMValueRef value, zero;

		value	= stack_pop(ctx->mimic_stack);

		zero	= LLVMConstInt(LLVMTypeOf(value), 0, 0);

		llvm_build_if(ctx, insn_start, code, pos, LLVMIntSGT, value, zero);

		break;
	}
	case OPC_IFLE: {
		LLVMValueRef value, zero;

		value	= stack_pop(ctx->mimic_stack);

		zero	= LLVMConstInt(LLVMTypeOf(value), 0, 0);

		llvm_build_if(ctx, insn_start, code, pos, LLVMIntSLE, value, zero);

		break;
	}
	case OPC_IF_ICMPEQ: {
		LLVMValueRef value1, value2;

		value2	= stack_pop(ctx->mimic_stack);
		value1	= stack_pop(ctx->mimic_stack);

		llvm_build_if(ctx, insn_start, code, pos, LLVMIntEQ, value1, value2);

		break;
	}
	case OPC_IF_ICMPNE: {
		LLVMValueRef value1, value2;

		value2	= stack_pop(ctx->mimic_stack);
		value1	= stack_pop(ctx->mimic_stack);

		llvm_build_if(ctx, insn_start, code, pos, LLVMIntNE, value1, value2);

		break;
	}
	case OPC_IF_ICMPLT: {
		LLVMValueRef value1, value2;

		value2	= stack_pop(ctx->mimic_stack);
		value1	= stack_pop(ctx->mimic_stack);

		llvm_build_if(ctx, insn_start, code, pos, LLVMIntSLT, value1, value2);

		break;
	}
	case OPC_IF_ICMPGE: {
		LLVMValueRef value1, value2;

		value2	= stack_pop(ctx->mimic_stack);
		value1	= stack_pop(ctx->mimic_stack);

		llvm_build_if(ctx, insn_start, code, pos, LLVMIntSGE, value1, value2);

		break;
	}
	case OPC_IF_ICMPGT: {
		LLVMValueRef value1, value2;

		value2	= stack_pop(ctx->mimic_stack);
		value1	= stack_pop(ctx->mimic_stack);

		llvm_build_if(ctx, insn_start, code, pos, LLVMIntSGT, value1, value2);

		break;
	}
	case OPC_IF_ICMPLE: {
		LLVMValueRef value1, value2;

		value2	= stack_pop(ctx->mimic_stack);
		value1	= stack_pop(ctx->mimic_stack);

		llvm_build_if(ctx, insn_start, code, pos, LLVMIntSLE, value1, value2);

		break;
	}
	case OPC_IF_ACMPEQ: {
		LLVMValueRef value1, value2;

		value2	= stack_pop(ctx->mimic_stack);
		value1	= stack_pop(ctx->mimic_stack);

		llvm_build_if(ctx, insn_start, code, pos, LLVMIntEQ, value1, value2);

		break;
	}
	case OPC_IF_ACMPNE: {
		LLVMValueRef value1, value2;

		value2	= stack_pop(ctx->mimic_stack);
		value1	= stack_pop(ctx->mimic_stack);

		llvm_build_if(ctx, insn_start, code, pos, LLVMIntNE, value1, value2);

		break;
	}
	case OPC_GOTO: {
		struct basic_block *goto_bb;
		int16_t offset;

		offset = read_s16(code, pos);

		goto_bb	= find_bb(ctx->cu, insn_start + offset);

		LLVMBuildBr(ctx->builder, goto_bb->priv);

		break;
	}
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

		value = stack_pop(ctx->mimic_stack);

		LLVMBuildRet(ctx->builder, value);

		stack_clear(ctx->mimic_stack);
		break;
	}
	case OPC_RETURN: {
		LLVMBuildRetVoid(ctx->builder);

		stack_clear(ctx->mimic_stack);
		break;
	}
	case OPC_GETSTATIC: {
		LLVMValueRef value, objectref, addr, gep;
		struct vm_class *source_vmc;
		LLVMValueRef indices[1];
		struct vm_field *vmf;
		LLVMTypeRef type;
		uint16_t idx;

		idx = read_u16(code, pos);

		vmf = vm_class_resolve_field_recursive(vmm->class, idx);

		assert(vmf != NULL);

		source_vmc = vmf->class;

		/*
		 * XXX: Use guard pages to eliminate the extra call on every
		 * field access.
		 */
		llvm_build_ensure_class_init(ctx, source_vmc);

		type = llvm_type(vmf->type_info.vm_type);

		objectref	= llvm_ptr_to_value(source_vmc->static_values, LLVMReferenceType());

		indices[0] = LLVMConstInt(LLVMInt32Type(), vmf->offset, 0);

		gep 	= LLVMBuildGEP(ctx->builder, objectref, indices, 1, "");

		addr	= LLVMBuildBitCast(ctx->builder, gep, LLVMPointerType(type, 0), "");

		value	= LLVMBuildLoad(ctx->builder, addr, "");

		stack_push(ctx->mimic_stack, value);

		break;
	}
	case OPC_PUTSTATIC: {
		LLVMValueRef objectref, addr, gep;
		struct vm_class *target_vmc;
		LLVMValueRef indices[1];
		LLVMValueRef target_in;
		struct vm_field *vmf;
		LLVMTypeRef type;
		uint16_t idx;

		target_in = stack_pop(ctx->mimic_stack);

		idx = read_u16(code, pos);

		vmf = vm_class_resolve_field_recursive(vmm->class, idx);

		assert(vmf != NULL);

		target_vmc = vmf->class;

		/*
		 * XXX: Use guard pages to eliminate the extra call on every
		 * field access.
		 */
		llvm_build_ensure_class_init(ctx, target_vmc);

		type = llvm_type(vmf->type_info.vm_type);

		objectref	= llvm_ptr_to_value(vmf->class->static_values, LLVMReferenceType());

		indices[0] = LLVMConstInt(LLVMInt32Type(), vmf->offset, 0);

		gep 	= LLVMBuildGEP(ctx->builder, objectref, indices, 1, "");

		addr	= LLVMBuildBitCast(ctx->builder, gep, LLVMPointerType(type, 0), "");

		LLVMBuildStore(ctx->builder, target_in, addr);

		break;
	}
	case OPC_GETFIELD: {
		LLVMValueRef value, objectref, addr, gep;
		struct vm_class *vmc = vmm->class;
		LLVMValueRef indices[1];
		struct vm_field *vmf;
		LLVMTypeRef type;
		uint16_t idx;

		idx = read_u16(code, pos);

		vmf = vm_class_resolve_field_recursive(vmc, idx);

		type = llvm_type(vmf->type_info.vm_type);

		assert(vmf != NULL);

		objectref	= stack_pop(ctx->mimic_stack);

		indices[0] = LLVMConstInt(LLVMInt32Type(), VM_OBJECT_FIELDS_OFFSET + vmf->offset, 0);

		gep 	= LLVMBuildGEP(ctx->builder, objectref, indices, 1, "");

		addr	= LLVMBuildBitCast(ctx->builder, gep, LLVMPointerType(type, 0), "");

		value	= LLVMBuildLoad(ctx->builder, addr, "");

		stack_push(ctx->mimic_stack, value);

		break;
	}
	case OPC_PUTFIELD: {
		LLVMValueRef value, objectref, addr, gep;
		struct vm_class *vmc = vmm->class;
		LLVMValueRef indices[1];
		struct vm_field *vmf;
		uint16_t idx;

		idx = read_u16(code, pos);

		vmf = vm_class_resolve_field_recursive(vmc, idx);

		assert(vmf != NULL);

		value		= stack_pop(ctx->mimic_stack);
		objectref	= stack_pop(ctx->mimic_stack);

		indices[0] = LLVMConstInt(LLVMInt32Type(), VM_OBJECT_FIELDS_OFFSET + vmf->offset, 0);

		gep 	= LLVMBuildGEP(ctx->builder, objectref, indices, 1, "");

		addr	= LLVMBuildBitCast(ctx->builder, gep, LLVMPointerType(LLVMTypeOf(value), 0), "");

		LLVMBuildStore(ctx->builder, value, addr);

		break;
	}
	case OPC_INVOKEVIRTUAL: {
		struct vm_method *target;
		unsigned long nr_args;
		LLVMValueRef value;
		LLVMValueRef *args;
		LLVMValueRef func;
		uint16_t idx;

		idx	= read_u16(code, pos);

		target	= vm_class_resolve_method_recursive(vmm->class, idx, 0);

		nr_args	= count_java_arguments(target);

		if (!vm_method_is_static(target))
			nr_args++;

		args	= llvm_convert_args(ctx, target, nr_args);

		func	= llvm_vtable_trampoline(ctx, target, args[0]);

		value	= LLVMBuildCall(ctx->builder, func, args, nr_args, "");

		free(args);

		/* XXX: Exception check */

		if (target->return_type.vm_type != J_VOID)
			stack_push(ctx->mimic_stack, value);

		break;
	}
	case OPC_INVOKESPECIAL: {
		struct vm_method *target;
		unsigned long nr_args;
		LLVMValueRef value;
		LLVMValueRef *args;
		LLVMValueRef func;
		uint16_t idx;

		idx	= read_u16(code, pos);

		target	= vm_class_resolve_method_recursive(vmm->class, idx, CAFEBABE_CLASS_ACC_STATIC);

		nr_args	= count_java_arguments(target);

		if (!vm_method_is_static(target))
			nr_args++;

		args	= llvm_convert_args(ctx, target, nr_args);

		func	= llvm_trampoline(target);

		value	= LLVMBuildCall(ctx->builder, func, args, nr_args, "");

		free(args);

		/* XXX: Exception check */

		if (target->return_type.vm_type != J_VOID)
			stack_push(ctx->mimic_stack, value);

		break;
	}
	case OPC_INVOKESTATIC: {
		struct vm_method *target;
		unsigned long nr_args;
		LLVMValueRef value;
		LLVMValueRef *args;
		LLVMValueRef func;
		uint16_t idx;

		idx	= read_u16(code, pos);

		target	= vm_class_resolve_method_recursive(vmm->class, idx, CAFEBABE_CLASS_ACC_STATIC);

		nr_args	= count_java_arguments(target);

		if (!vm_method_is_static(target))
			nr_args++;

		args	= llvm_convert_args(ctx, target, nr_args);

		func	= llvm_trampoline(target);

		value	= LLVMBuildCall(ctx->builder, func, args, nr_args, "");

		free(args);

		/* XXX: Exception check */

		if (target->return_type.vm_type != J_VOID)
			stack_push(ctx->mimic_stack, value);

		break;
	}
	case OPC_INVOKEINTERFACE: {
		struct vm_method *target_vmm;
		unsigned long nr_args;
		LLVMValueRef value;
		LLVMValueRef *args;
		LLVMValueRef func;
		uint16_t idx;

		idx = read_u16(code, pos);

		/* Skip count. */		
		read_u8(code, pos);

		/* Skip zero. */
		read_u8(code, pos);

		target_vmm = vm_class_resolve_interface_method_recursive(vmm->class, idx);

		assert(target_vmm != NULL);

		nr_args	= count_java_arguments(target_vmm);

		if (!vm_method_is_static(target_vmm))
			nr_args++;

		args	= llvm_convert_args(ctx, target_vmm, nr_args);

		func	= llvm_itable_trampoline(ctx, target_vmm, args[0], idx);

		value	= LLVMBuildCall(ctx->builder, func, args, nr_args, "");

		free(args);

		/* XXX: Exception check */

		if (target_vmm->return_type.vm_type != J_VOID)
			stack_push(ctx->mimic_stack, value);

		break;
	}
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
	case OPC_NEWARRAY: {
		LLVMValueRef arrayref;
		LLVMValueRef args[2];
		LLVMValueRef count;
		uint8_t atype;

		atype = read_u8(code, pos);

		count = stack_pop(ctx->mimic_stack);

		args[0] = LLVMConstInt(LLVMInt32Type(), atype, 0);

		args[1] = count;

		arrayref = LLVMBuildCall(ctx->builder, vm_object_alloc_primitive_array_func, args, 2, "");

		/* XXX: Exception check */

		stack_push(ctx->mimic_stack, arrayref);

		break;
	}
	case OPC_ANEWARRAY: {
		LLVMValueRef arrayref;
		struct vm_class *vmc;
		LLVMValueRef args[2];
		LLVMValueRef count;
		uint16_t idx;

		idx = read_u16(code, pos);

		vmc = vm_class_resolve_class(vmm->class, idx);

		count = stack_pop(ctx->mimic_stack);

		assert(vmc != NULL);

		args[0] = llvm_ptr_to_value(vmc, LLVMReferenceType());

		args[1] = count;

		arrayref = LLVMBuildCall(ctx->builder, vm_object_alloc_array_func, args, 2, "");

		/* XXX: Exception check */

		stack_push(ctx->mimic_stack, arrayref);

		break;
	}
	case OPC_ARRAYLENGTH: {
		LLVMValueRef arrayref, length, addr, gep;
		LLVMValueRef indices[1];

		arrayref = stack_pop(ctx->mimic_stack);

		indices[0] = LLVMConstInt(LLVMInt32Type(), offsetof(struct vm_array, array_length), 0);

		gep 	= LLVMBuildGEP(ctx->builder, arrayref, indices, 1, "");

		addr	= LLVMBuildBitCast(ctx->builder, gep, LLVMPointerType(LLVMInt32Type(), 0), "");

		length	= LLVMBuildLoad(ctx->builder, addr, "");

		stack_push(ctx->mimic_stack, length);

		break;
	}
	case OPC_ATHROW: {
		LLVMValueRef objectref;
		LLVMValueRef args[1];
		LLVMValueRef call;

		objectref = stack_pop(ctx->mimic_stack);

		stack_clear(ctx->mimic_stack);

		args[0]	= objectref;

		call	= LLVMBuildCall(ctx->builder, llvm_throw_stub_func, args, 1, "");

		LLVMSetTailCall(call, 1);

		break;
	}
	case OPC_CHECKCAST: {
		LLVMValueRef objectref;
		struct vm_class *vmc;
		LLVMValueRef args[2];
		uint16_t idx;

		idx = read_u16(code, pos);

		vmc = vm_class_resolve_class(vmm->class, idx);

		assert(vmc != NULL);

		objectref = stack_peek(ctx->mimic_stack);

		args[0] = objectref;

		args[1] = llvm_ptr_to_value(vmc, LLVMReferenceType());

		LLVMBuildCall(ctx->builder, vm_object_check_cast_func, args, 2, "");

		break;
	}
	case OPC_INSTANCEOF: {
		LLVMValueRef objectref;
		struct vm_class *vmc;
		LLVMValueRef args[2];
		LLVMValueRef value;
		uint16_t idx;

		idx = read_u16(code, pos);

		vmc = vm_class_resolve_class(vmm->class, idx);

		assert(vmc != NULL);

		objectref = stack_peek(ctx->mimic_stack);

		args[0] = objectref;

		args[1] = llvm_ptr_to_value(vmc, LLVMReferenceType());

		value = LLVMBuildCall(ctx->builder, vm_object_is_instance_of_func, args, 2, "");

		stack_push(ctx->mimic_stack, value);

		break;
	}
	case OPC_MONITORENTER: {
		LLVMValueRef objectref;

		objectref = stack_pop(ctx->mimic_stack);

		llvm_build_monitorenter(ctx, objectref);

		break;
	}
	case OPC_MONITOREXIT: {
		LLVMValueRef objectref;

		objectref = stack_pop(ctx->mimic_stack);

		llvm_build_monitorexit(ctx, objectref);

		break;
	}
	case OPC_WIDE: {
		wide = true;

		goto restart;
	}
	case OPC_MULTIANEWARRAY:	assert(0); break;
	case OPC_IFNULL: {
		LLVMValueRef value, null;

		value	= stack_pop(ctx->mimic_stack);

		null	= LLVMConstNull(LLVMTypeOf(value));

		llvm_build_if(ctx, insn_start, code, pos, LLVMIntEQ, value, null);

		break;
	}
	case OPC_IFNONNULL: {
		LLVMValueRef value, null;

		value	= stack_pop(ctx->mimic_stack);

		null	= LLVMConstNull(LLVMTypeOf(value));

		llvm_build_if(ctx, insn_start, code, pos, LLVMIntNE, value, null);

		break;
	}
	case OPC_GOTO_W: {
		struct basic_block *goto_bb;
		int32_t offset;

		offset = read_s32(code, pos);

		goto_bb	= find_bb(ctx->cu, insn_start + offset);

		LLVMBuildBr(ctx->builder, goto_bb->priv);

		break;
	}
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
	struct vm_method *vmm = cu->method;
	struct basic_block *bb, *prev;
	LLVMBasicBlockRef bbr;

	ctx->builder = LLVMCreateBuilder();

	for_each_basic_block(bb, &cu->bb_list) {
		bbr = LLVMAppendBasicBlock(ctx->func, "L");

		bb->priv = bbr;
	}

	bbr = cu->entry_bb->priv;

	LLVMPositionBuilderAtEnd(ctx->builder, bbr);

	if (method_is_synchronized(vmm)) {
		LLVMValueRef objectref;

		if (vm_method_is_static(vmm))
			objectref = llvm_ptr_to_value(vmm->class->object, LLVMReferenceType());
		else
			objectref = LLVMGetParam(ctx->func, 0);

		llvm_build_monitorenter(ctx, objectref);
	}

	llvm_bc2ir_bb(ctx, cu->entry_bb);

	prev = cu->entry_bb;

	for_each_basic_block(bb, &cu->bb_list) {
		bbr = bb->priv;

		if (bb->is_converted)
			continue;

		if (!prev->has_branch && !prev->has_return)
			LLVMBuildBr(ctx->builder, bbr);

		if (bb->is_eh)
			stack_push(ctx->mimic_stack, LLVMConstNull(LLVMReferenceType()));

		LLVMPositionBuilderAtEnd(ctx->builder, bbr);

		llvm_bc2ir_bb(ctx, bb);

		prev = bb;
	}

	LLVMPositionBuilderAtEnd(ctx->builder, cu->exit_bb->priv);

	if (method_is_synchronized(vmm)) {
		LLVMValueRef objectref;

		if (vm_method_is_static(vmm))
			objectref = llvm_ptr_to_value(vmm->class->object, LLVMReferenceType());
		else
			objectref = LLVMGetParam(ctx->func, 0);

		llvm_build_monitorexit(ctx, objectref);
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

	LLVMVerifyModule(module, LLVMAbortProcessAction, NULL);

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

	if (opt_llvm_verbose) {
		trace_printf("Compiling %s.%s%s...\n", vmc->name, vmm->name, vmm->type);
		trace_bytecode(vmm);
		trace_flush();
	}

	err = inline_subroutines(cu->method);
	if (err)
		goto out;

	err = analyze_control_flow(cu);
	if (err)
		goto out;

	err = llvm_codegen(cu);
	if (err)
		goto out;

out:
	return err;
}

static LLVMValueRef llvm_setup_func(const char *name, void *func_p, enum vm_type return_type, unsigned args_count, ...)
{
	LLVMTypeRef arg_types[args_count];
	LLVMTypeRef func_type;
	LLVMValueRef func;
	unsigned idx;
	va_list ap;

	va_start(ap, args_count);

	for (idx = 0; idx < args_count; idx++) {
		arg_types[idx]	= llvm_type(va_arg(ap, enum vm_type));
	}

	va_end(ap);

	func_type	= LLVMFunctionType(llvm_type(return_type), arg_types, args_count, 0);

	func		= LLVMAddFunction(module, name, func_type);

	LLVMAddGlobalMapping(engine, func, func_p);

	return func;
}

static void llvm_setup_builtins(void)
{
	LLVM_DEFINE_BUILTIN(vm_class_ensure_init, J_INT, 1, J_REFERENCE);
	LLVM_DEFINE_BUILTIN(vm_object_alloc, J_REFERENCE, 1, J_REFERENCE);
	LLVM_DEFINE_BUILTIN(vm_object_alloc_array, J_REFERENCE, 2, J_REFERENCE, J_INT);
	LLVM_DEFINE_BUILTIN(vm_object_alloc_primitive_array, J_REFERENCE, 2, J_INT, J_INT);
	LLVM_DEFINE_BUILTIN(vm_object_alloc_string_from_utf8, J_REFERENCE, 2, J_REFERENCE, J_INT);
	LLVM_DEFINE_BUILTIN(vm_object_check_cast, J_VOID, 2, J_REFERENCE, J_REFERENCE);
	LLVM_DEFINE_BUILTIN(vm_object_is_instance_of, J_INT, 2, J_REFERENCE, J_REFERENCE);
	LLVM_DEFINE_BUILTIN(vm_object_lock, J_INT, 1, J_REFERENCE);
	LLVM_DEFINE_BUILTIN(vm_object_unlock, J_INT, 1, J_REFERENCE);
	LLVM_DEFINE_BUILTIN(emulate_dcmpg, J_INT, 2, J_DOUBLE, J_DOUBLE);
	LLVM_DEFINE_BUILTIN(emulate_dcmpl, J_INT, 2, J_DOUBLE, J_DOUBLE);
	LLVM_DEFINE_BUILTIN(emulate_fcmpg, J_INT, 2, J_FLOAT, J_FLOAT);
	LLVM_DEFINE_BUILTIN(emulate_fcmpl, J_INT, 2, J_FLOAT, J_FLOAT);
	LLVM_DEFINE_BUILTIN(emulate_lcmp, J_INT, 2, J_LONG, J_LONG);
	LLVM_DEFINE_BUILTIN(llvm_throw_stub, J_VOID, 1, J_REFERENCE);
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

	llvm_setup_builtins();
}

void llvm_exit(void)
{
	LLVMDisposeExecutionEngine(engine);

	LLVMDisposeModule(module);
}
