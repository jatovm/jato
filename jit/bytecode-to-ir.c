/*
 * Copyright (C) 2005-2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 *
 * The file contains functions for converting Java bytecode to immediate
 * representation of the JIT compiler.
 */

#include <jit/statement.h>
#include <vm/stack.h>
#include <jit-compiler.h>
#include <bytecodes.h>
#include <jit/bytecode-converters.h>
#include <vm/const-pool.h>

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static int convert_nop(struct compilation_unit *cu, struct basic_block *bb,
		       unsigned long offset)
{
	struct statement *stmt;

	stmt = alloc_statement(STMT_NOP);
	if (!stmt)
		return -ENOMEM;

	bb_add_stmt(bb, stmt);
	return 0;
}

typedef int (*convert_fn_t) (struct compilation_unit *, struct basic_block *,
			     unsigned long);

static convert_fn_t converters[] = {
	[OPC_NOP] = convert_nop,
	[OPC_ACONST_NULL] = convert_aconst_null,
	[OPC_ICONST_M1] = convert_iconst,
	[OPC_ICONST_0] = convert_iconst,
	[OPC_ICONST_1] = convert_iconst,
	[OPC_ICONST_2] = convert_iconst,
	[OPC_ICONST_3] = convert_iconst,
	[OPC_ICONST_4] = convert_iconst,
	[OPC_ICONST_5] = convert_iconst,
	[OPC_LCONST_0] = convert_lconst,
	[OPC_LCONST_1] = convert_lconst,
	[OPC_FCONST_0] = convert_fconst,
	[OPC_FCONST_1] = convert_fconst,
	[OPC_FCONST_2] = convert_fconst,
	[OPC_DCONST_0] = convert_dconst,
	[OPC_DCONST_1] = convert_dconst,
	[OPC_BIPUSH] = convert_bipush,
	[OPC_SIPUSH] = convert_sipush,
	[OPC_LDC] = convert_ldc,
	[OPC_LDC_W] = convert_ldc_w,
	[OPC_LDC2_W] = convert_ldc2_w,
	[OPC_ILOAD] = convert_iload,
	[OPC_LLOAD] = convert_lload,
	[OPC_FLOAD] = convert_fload,
	[OPC_DLOAD] = convert_dload,
	[OPC_ALOAD] = convert_aload,
	[OPC_ILOAD_0] = convert_iload_n,
	[OPC_ILOAD_1] = convert_iload_n,
	[OPC_ILOAD_2] = convert_iload_n,
	[OPC_ILOAD_3] = convert_iload_n,
	[OPC_LLOAD_0] = convert_lload_n,
	[OPC_LLOAD_1] = convert_lload_n,
	[OPC_LLOAD_2] = convert_lload_n,
	[OPC_LLOAD_3] = convert_lload_n,
	[OPC_FLOAD_0] = convert_fload_n,
	[OPC_FLOAD_1] = convert_fload_n,
	[OPC_FLOAD_2] = convert_fload_n,
	[OPC_FLOAD_3] = convert_fload_n,
	[OPC_DLOAD_0] = convert_dload_n,
	[OPC_DLOAD_1] = convert_dload_n,
	[OPC_DLOAD_2] = convert_dload_n,
	[OPC_DLOAD_3] = convert_dload_n,
	[OPC_ALOAD_0] = convert_aload_n,
	[OPC_ALOAD_1] = convert_aload_n,
	[OPC_ALOAD_2] = convert_aload_n,
	[OPC_ALOAD_3] = convert_aload_n,
	[OPC_IALOAD] = convert_iaload,
	[OPC_LALOAD] = convert_laload,
	[OPC_FALOAD] = convert_faload,
	[OPC_DALOAD] = convert_daload,
	[OPC_AALOAD] = convert_aaload,
	[OPC_BALOAD] = convert_baload,
	[OPC_CALOAD] = convert_caload,
	[OPC_SALOAD] = convert_saload,
	[OPC_ISTORE] = convert_istore,
	[OPC_LSTORE] = convert_lstore,
	[OPC_FSTORE] = convert_fstore,
	[OPC_DSTORE] = convert_dstore,
	[OPC_ASTORE] = convert_astore,
	[OPC_ISTORE_0] = convert_istore_n,
	[OPC_ISTORE_1] = convert_istore_n,
	[OPC_ISTORE_2] = convert_istore_n,
	[OPC_ISTORE_3] = convert_istore_n,
	[OPC_LSTORE_0] = convert_lstore_n,
	[OPC_LSTORE_1] = convert_lstore_n,
	[OPC_LSTORE_2] = convert_lstore_n,
	[OPC_LSTORE_3] = convert_lstore_n,
	[OPC_FSTORE_0] = convert_fstore_n,
	[OPC_FSTORE_1] = convert_fstore_n,
	[OPC_FSTORE_2] = convert_fstore_n,
	[OPC_FSTORE_3] = convert_fstore_n,
	[OPC_DSTORE_0] = convert_dstore_n,
	[OPC_DSTORE_1] = convert_dstore_n,
	[OPC_DSTORE_2] = convert_dstore_n,
	[OPC_DSTORE_3] = convert_dstore_n,
	[OPC_ASTORE_0] = convert_astore_n,
	[OPC_ASTORE_1] = convert_astore_n,
	[OPC_ASTORE_2] = convert_astore_n,
	[OPC_ASTORE_3] = convert_astore_n,
	[OPC_IASTORE] = convert_iastore,
	[OPC_LASTORE] = convert_lastore,
	[OPC_FASTORE] = convert_fastore,
	[OPC_DASTORE] = convert_dastore,
	[OPC_AASTORE] = convert_aastore,
	[OPC_BASTORE] = convert_bastore,
	[OPC_CASTORE] = convert_castore,
	[OPC_SASTORE] = convert_sastore,
	[OPC_POP] = convert_pop,
	[OPC_POP2] = convert_pop,
	[OPC_DUP] = convert_dup,
	[OPC_DUP_X1] = convert_dup_x1,
	[OPC_DUP_X2] = convert_dup_x2,
	[OPC_DUP2] = convert_dup,
	[OPC_DUP2_X1] = convert_dup_x1,
	[OPC_DUP2_X2] = convert_dup_x2,
	[OPC_SWAP] = convert_swap,
	[OPC_IADD] = convert_iadd,
	[OPC_LADD] = convert_ladd,
	[OPC_FADD] = convert_fadd,
	[OPC_DADD] = convert_dadd,
	[OPC_ISUB] = convert_isub,
	[OPC_LSUB] = convert_lsub,
	[OPC_FSUB] = convert_fsub,
	[OPC_DSUB] = convert_dsub,
	[OPC_IMUL] = convert_imul,
	[OPC_LMUL] = convert_lmul,
	[OPC_FMUL] = convert_fmul,
	[OPC_DMUL] = convert_dmul,
	[OPC_IDIV] = convert_idiv,
	[OPC_LDIV] = convert_ldiv,
	[OPC_FDIV] = convert_fdiv,
	[OPC_DDIV] = convert_ddiv,
	[OPC_IREM] = convert_irem,
	[OPC_LREM] = convert_lrem,
	[OPC_FREM] = convert_frem,
	[OPC_DREM] = convert_drem,
	[OPC_INEG] = convert_ineg,
	[OPC_LNEG] = convert_lneg,
	[OPC_FNEG] = convert_fneg,
	[OPC_DNEG] = convert_dneg,
	[OPC_ISHL] = convert_ishl,
	[OPC_LSHL] = convert_lshl,
	[OPC_ISHR] = convert_ishr,
	[OPC_LSHR] = convert_lshr,
	[OPC_IAND] = convert_iand,
	[OPC_LAND] = convert_land,
	[OPC_IOR] = convert_ior,
	[OPC_LOR] = convert_lor,
	[OPC_IXOR] = convert_ixor,
	[OPC_LXOR] = convert_lxor,
	[OPC_IINC] = convert_iinc,
	[OPC_I2L] = convert_i2l,
	[OPC_I2F] = convert_i2f,
	[OPC_I2D] = convert_i2d,
	[OPC_L2I] = convert_l2i,
	[OPC_L2F] = convert_l2f,
	[OPC_L2D] = convert_l2d,
	[OPC_F2I] = convert_f2i,
	[OPC_F2L] = convert_f2l,
	[OPC_F2D] = convert_f2d,
	[OPC_D2I] = convert_d2i,
	[OPC_D2L] = convert_d2l,
	[OPC_D2F] = convert_d2f,
	[OPC_I2B] = convert_i2b,
	[OPC_I2C] = convert_i2c,
	[OPC_I2S] = convert_i2s,
	[OPC_LCMP] = convert_lcmp,
	[OPC_FCMPL] = convert_xcmpl,
	[OPC_FCMPG] = convert_xcmpg,
	[OPC_DCMPL] = convert_xcmpl,
	[OPC_DCMPG] = convert_xcmpg,
	[OPC_IFEQ] = convert_ifeq,
	[OPC_IFNE] = convert_ifne,
	[OPC_IFLT] = convert_iflt,
	[OPC_IFGE] = convert_ifge,
	[OPC_IFGT] = convert_ifgt,
	[OPC_IFLE] = convert_ifle,
	[OPC_IF_ICMPEQ] = convert_if_icmpeq,
	[OPC_IF_ICMPNE] = convert_if_icmpne,
	[OPC_IF_ICMPLT] = convert_if_icmplt,
	[OPC_IF_ICMPGE] = convert_if_icmpge,
	[OPC_IF_ICMPGT] = convert_if_icmpgt,
	[OPC_IF_ICMPLE] = convert_if_icmple,
	[OPC_IF_ACMPEQ] = convert_if_acmpeq,
	[OPC_IF_ACMPNE] = convert_if_acmpne,
	[OPC_GOTO] = convert_goto,
	[OPC_IRETURN] = convert_non_void_return,
	[OPC_LRETURN] = convert_non_void_return,
	[OPC_FRETURN] = convert_non_void_return,
	[OPC_DRETURN] = convert_non_void_return,
	[OPC_ARETURN] = convert_non_void_return,
	[OPC_RETURN] = convert_void_return,
	[OPC_GETSTATIC] = convert_getstatic,
	[OPC_PUTSTATIC] = convert_putstatic,
	[OPC_INVOKEVIRTUAL] = convert_invokevirtual,
	[OPC_INVOKESTATIC] = convert_invokestatic,
	[OPC_IFNULL] = convert_ifnull,
	[OPC_IFNONNULL] = convert_ifnonnull,
};

/**
 *	convert_to_ir - Convert bytecode to intermediate representation.
 *	@compilation_unit: compilation unit to convert.
 *
 *	This function converts bytecode in a compilation unit to intermediate
 *	representation of the JIT compiler.
 *
 *	Returns zero if conversion succeeded; otherwise returns a negative
 * 	integer.
 */
int convert_to_ir(struct compilation_unit *cu)
{
	int err = 0;
	unsigned long code_size = cu->method->code_size;
	unsigned char *code = cu->method->jit_code;
	unsigned long offset = 0;
	struct basic_block *entry_bb;

	entry_bb = list_entry(cu->bb_list.next, struct basic_block, bb_list_node);

	while (offset < code_size) {
		unsigned char opc = code[offset];
		unsigned long opc_size;
		convert_fn_t convert;

		convert = converters[opc];
		if (!convert) {
			printf("%s: Unknown bytecode instruction 0x%x in "
			       "method '%s' at offset %lu.\n",
			       __FUNCTION__, opc, cu->method->name, offset);
			err = -EINVAL;
			goto out;
		}

		opc_size = bytecode_size(code + offset);
		if (opc_size > code_size-offset) {
			printf("%s: Premature end of bytecode stream in "
			       "method '%s' (code_size: %lu, offset: %lu, "
			       "opc_size: %lu, opc: 0x%x)\n.",
			       __FUNCTION__, cu->method->name, code_size,
			       offset, opc_size, opc);
			err = -EINVAL;
			goto out;
		}
	
		err = convert(cu, entry_bb, offset);
		if (err)
			goto out;

		offset += opc_size;
	}
  out:
	return err;
}
