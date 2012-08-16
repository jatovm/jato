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

/*
 * A helper function that looks like LLVM API for JVM reference types.
 * We treat them as opaque pointers much like 'void *' in C.
 */
static inline LLVMTypeRef LLVMReferenceType(void)
{
	return LLVMPointerType(LLVMInt8Type(), 0);
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

static LLVMTypeRef llvm_function_type(struct llvm_context *ctx)
{
	struct vm_method *vmm = ctx->cu->method;
	unsigned long ndx = 0, args_count = 0;
	LLVMTypeRef *arg_types = NULL;
	struct vm_method_arg *arg;
	LLVMTypeRef return_type;
	LLVMTypeRef func_type;

	list_for_each_entry(arg, &vmm->args, list_node) {
		args_count++;
	}

	if (!args_count)
		goto skip_args;

	arg_types = calloc(1, args_count);
	if (!arg_types)
		return NULL;

	list_for_each_entry(arg, &vmm->args, list_node) {
		arg_types[ndx++] = llvm_type(arg->type_info.vm_type);
	}

skip_args:

	return_type = llvm_type(vmm->return_type.vm_type);

	func_type = LLVMFunctionType(return_type, arg_types, args_count, 0);

	free(arg_types);

	return func_type;
}

static void llvm_function_name(struct llvm_context *ctx, char *name, size_t len)
{
	struct vm_method *vmm = ctx->cu->method;
	char *p;

	snprintf(name, len, "%s.%s%s", vmm->class->name, vmm->name, vmm->type);

	for (p = name; *p; p++)
		if (!isalnum(*p))
			*p = '_';
}

static LLVMValueRef llvm_function(struct llvm_context *ctx)
{
	LLVMTypeRef func_type;
	char func_name[1024];

	llvm_function_name(ctx, func_name, sizeof(func_name));

	func_type = llvm_function_type(ctx);

	return LLVMAddFunction(module, func_name, func_type);
}

static int llvm_bc2ir_insn(struct llvm_context *ctx, unsigned char *code, unsigned long *idx)
{
	struct vm_method *vmm = ctx->cu->method;
	unsigned char opc;

	opc = code[*idx++];

	switch (opc) {
	case OPC_NOP:			assert(0); break;
	case OPC_ACONST_NULL:		assert(0); break;
	case OPC_ICONST_M1:		assert(0); break;
	case OPC_ICONST_0:		assert(0); break;
	case OPC_ICONST_1:		assert(0); break;
	case OPC_ICONST_2:		assert(0); break;
	case OPC_ICONST_3:		assert(0); break;
	case OPC_ICONST_4:		assert(0); break;
	case OPC_ICONST_5:		assert(0); break;
	case OPC_LCONST_0:		assert(0); break;
	case OPC_LCONST_1:		assert(0); break;
	case OPC_FCONST_0:		assert(0); break;
	case OPC_FCONST_1:		assert(0); break;
	case OPC_FCONST_2:		assert(0); break;
	case OPC_DCONST_0:		assert(0); break;
	case OPC_DCONST_1:		assert(0); break;
	case OPC_BIPUSH:		assert(0); break;
	case OPC_SIPUSH:		assert(0); break;
	case OPC_LDC:			assert(0); break;
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
	case OPC_ALOAD_0:		assert(0); break;
	case OPC_ALOAD_1:		assert(0); break;
	case OPC_ALOAD_2:		assert(0); break;
	case OPC_ALOAD_3:		assert(0); break;
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
	case OPC_POP:			assert(0); break;
	case OPC_POP2:			assert(0); break;
	case OPC_DUP:			assert(0); break;
	case OPC_DUP_X1:		assert(0); break;
	case OPC_DUP_X2:		assert(0); break;
	case OPC_DUP2:			assert(0); break;
	case OPC_DUP2_X1:		assert(0); break;
	case OPC_DUP2_X2:		assert(0); break;
	case OPC_SWAP:			assert(0); break;
	case OPC_IADD:			assert(0); break;
	case OPC_LADD:			assert(0); break;
	case OPC_FADD:			assert(0); break;
	case OPC_DADD:			assert(0); break;
	case OPC_ISUB:			assert(0); break;
	case OPC_LSUB:			assert(0); break;
	case OPC_FSUB:			assert(0); break;
	case OPC_DSUB:			assert(0); break;
	case OPC_IMUL:			assert(0); break;
	case OPC_LMUL:			assert(0); break;
	case OPC_FMUL:			assert(0); break;
	case OPC_DMUL:			assert(0); break;
	case OPC_IDIV:			assert(0); break;
	case OPC_LDIV:			assert(0); break;
	case OPC_FDIV:			assert(0); break;
	case OPC_DDIV:			assert(0); break;
	case OPC_IREM:			assert(0); break;
	case OPC_LREM:			assert(0); break;
	case OPC_FREM:			assert(0); break;
	case OPC_DREM:			assert(0); break;
	case OPC_INEG:			assert(0); break;
	case OPC_LNEG:			assert(0); break;
	case OPC_FNEG:			assert(0); break;
	case OPC_DNEG:			assert(0); break;
	case OPC_ISHL:			assert(0); break;
	case OPC_LSHL:			assert(0); break;
	case OPC_ISHR:			assert(0); break;
	case OPC_LSHR:			assert(0); break;
	case OPC_IUSHR:			assert(0); break;
	case OPC_LUSHR:			assert(0); break;
	case OPC_IAND:			assert(0); break;
	case OPC_LAND:			assert(0); break;
	case OPC_IOR:			assert(0); break;
	case OPC_LOR:			assert(0); break;
	case OPC_IXOR:			assert(0); break;
	case OPC_LXOR:			assert(0); break;
	case OPC_IINC:			assert(0); break;
	case OPC_I2L:			assert(0); break;
	case OPC_I2F:			assert(0); break;
	case OPC_I2D:			assert(0); break;
	case OPC_L2I:			assert(0); break;
	case OPC_L2F:			assert(0); break;
	case OPC_L2D:			assert(0); break;
	case OPC_F2I:			assert(0); break;
	case OPC_F2L:			assert(0); break;
	case OPC_F2D:			assert(0); break;
	case OPC_D2I:			assert(0); break;
	case OPC_D2L:			assert(0); break;
	case OPC_D2F:			assert(0); break;
	case OPC_I2B:			assert(0); break;
	case OPC_I2C:			assert(0); break;
	case OPC_I2S:			assert(0); break;
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
	case OPC_IRETURN:		assert(0); break;
	case OPC_LRETURN:		assert(0); break;
	case OPC_FRETURN:		assert(0); break;
	case OPC_DRETURN:		assert(0); break;
	case OPC_ARETURN:		assert(0); break;
	case OPC_RETURN: {
		/* XXX: Update monitor state properly before exiting. */
		assert(!method_is_synchronized(vmm));

		LLVMBuildRetVoid(ctx->builder);
		break;
	}
	case OPC_GETSTATIC:		assert(0); break;
	case OPC_PUTSTATIC:		assert(0); break;
	case OPC_GETFIELD:		assert(0); break;
	case OPC_PUTFIELD:		assert(0); break;
	case OPC_INVOKEVIRTUAL:		assert(0); break;
	case OPC_INVOKESPECIAL:		assert(0); break;
	case OPC_INVOKESTATIC:		assert(0); break;
	case OPC_INVOKEINTERFACE:	assert(0); break;
	case OPC_NEW:			assert(0); break;
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
	unsigned long idx;

	code = vmm->code_attribute.code;

	idx = bb->start;

	while (idx < bb->end)
		llvm_bc2ir_insn(ctx, code, &idx);

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
	struct llvm_context ctx = {
		.cu = cu,
	};

	ctx.func = llvm_function(&ctx);
	if (!ctx.func)
		return -1;

	if (llvm_bc2ir(&ctx) < 0)
		assert(0);

	cu->entry_point = LLVMRecompileAndRelinkFunction(engine, ctx.func);

	return 0;
}

int llvm_compile(struct compilation_unit *cu)
{
	int err;

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
}

void llvm_exit(void)
{
	LLVMDisposeExecutionEngine(engine);

	LLVMDisposeModule(module);
}
