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

	bb_insert_stmt(bb, stmt);
	return 0;
}

typedef int (*convert_fn_t) (struct compilation_unit *, struct basic_block *,
			     unsigned long);

#define MAP_BYTECODE_TO(opc, fn) [opc] = fn

static convert_fn_t converters[] = {
	MAP_BYTECODE_TO(OPC_NOP, convert_nop),
	MAP_BYTECODE_TO(OPC_ACONST_NULL, convert_aconst_null),
	MAP_BYTECODE_TO(OPC_ICONST_M1, convert_iconst),
	MAP_BYTECODE_TO(OPC_ICONST_0, convert_iconst),
	MAP_BYTECODE_TO(OPC_ICONST_1, convert_iconst),
	MAP_BYTECODE_TO(OPC_ICONST_2, convert_iconst),
	MAP_BYTECODE_TO(OPC_ICONST_3, convert_iconst),
	MAP_BYTECODE_TO(OPC_ICONST_4, convert_iconst),
	MAP_BYTECODE_TO(OPC_ICONST_5, convert_iconst),
	MAP_BYTECODE_TO(OPC_LCONST_0, convert_lconst),
	MAP_BYTECODE_TO(OPC_LCONST_1, convert_lconst),
	MAP_BYTECODE_TO(OPC_FCONST_0, convert_fconst),
	MAP_BYTECODE_TO(OPC_FCONST_1, convert_fconst),
	MAP_BYTECODE_TO(OPC_FCONST_2, convert_fconst),
	MAP_BYTECODE_TO(OPC_DCONST_0, convert_dconst),
	MAP_BYTECODE_TO(OPC_DCONST_1, convert_dconst),
	MAP_BYTECODE_TO(OPC_BIPUSH, convert_bipush),
	MAP_BYTECODE_TO(OPC_SIPUSH, convert_sipush),
	MAP_BYTECODE_TO(OPC_LDC, convert_ldc),
	MAP_BYTECODE_TO(OPC_LDC_W, convert_ldc_w),
	MAP_BYTECODE_TO(OPC_LDC2_W, convert_ldc2_w),
	MAP_BYTECODE_TO(OPC_ILOAD, convert_iload),
	MAP_BYTECODE_TO(OPC_LLOAD, convert_lload),
	MAP_BYTECODE_TO(OPC_FLOAD, convert_fload),
	MAP_BYTECODE_TO(OPC_DLOAD, convert_dload),
	MAP_BYTECODE_TO(OPC_ALOAD, convert_aload),
	MAP_BYTECODE_TO(OPC_ILOAD_0, convert_iload_n),
	MAP_BYTECODE_TO(OPC_ILOAD_1, convert_iload_n),
	MAP_BYTECODE_TO(OPC_ILOAD_2, convert_iload_n),
	MAP_BYTECODE_TO(OPC_ILOAD_3, convert_iload_n),
	MAP_BYTECODE_TO(OPC_LLOAD_0, convert_lload_n),
	MAP_BYTECODE_TO(OPC_LLOAD_1, convert_lload_n),
	MAP_BYTECODE_TO(OPC_LLOAD_2, convert_lload_n),
	MAP_BYTECODE_TO(OPC_LLOAD_3, convert_lload_n),
	MAP_BYTECODE_TO(OPC_FLOAD_0, convert_fload_n),
	MAP_BYTECODE_TO(OPC_FLOAD_1, convert_fload_n),
	MAP_BYTECODE_TO(OPC_FLOAD_2, convert_fload_n),
	MAP_BYTECODE_TO(OPC_FLOAD_3, convert_fload_n),
	MAP_BYTECODE_TO(OPC_DLOAD_0, convert_dload_n),
	MAP_BYTECODE_TO(OPC_DLOAD_1, convert_dload_n),
	MAP_BYTECODE_TO(OPC_DLOAD_2, convert_dload_n),
	MAP_BYTECODE_TO(OPC_DLOAD_3, convert_dload_n),
	MAP_BYTECODE_TO(OPC_ALOAD_0, convert_aload_n),
	MAP_BYTECODE_TO(OPC_ALOAD_1, convert_aload_n),
	MAP_BYTECODE_TO(OPC_ALOAD_2, convert_aload_n),
	MAP_BYTECODE_TO(OPC_ALOAD_3, convert_aload_n),
	MAP_BYTECODE_TO(OPC_IALOAD, convert_iaload),
	MAP_BYTECODE_TO(OPC_LALOAD, convert_laload),
	MAP_BYTECODE_TO(OPC_FALOAD, convert_faload),
	MAP_BYTECODE_TO(OPC_DALOAD, convert_daload),
	MAP_BYTECODE_TO(OPC_AALOAD, convert_aaload),
	MAP_BYTECODE_TO(OPC_BALOAD, convert_baload),
	MAP_BYTECODE_TO(OPC_CALOAD, convert_caload),
	MAP_BYTECODE_TO(OPC_SALOAD, convert_saload),
	MAP_BYTECODE_TO(OPC_ISTORE, convert_istore),
	MAP_BYTECODE_TO(OPC_LSTORE, convert_lstore),
	MAP_BYTECODE_TO(OPC_FSTORE, convert_fstore),
	MAP_BYTECODE_TO(OPC_DSTORE, convert_dstore),
	MAP_BYTECODE_TO(OPC_ASTORE, convert_astore),
	MAP_BYTECODE_TO(OPC_ISTORE_0, convert_istore_n),
	MAP_BYTECODE_TO(OPC_ISTORE_1, convert_istore_n),
	MAP_BYTECODE_TO(OPC_ISTORE_2, convert_istore_n),
	MAP_BYTECODE_TO(OPC_ISTORE_3, convert_istore_n),
	MAP_BYTECODE_TO(OPC_LSTORE_0, convert_lstore_n),
	MAP_BYTECODE_TO(OPC_LSTORE_1, convert_lstore_n),
	MAP_BYTECODE_TO(OPC_LSTORE_2, convert_lstore_n),
	MAP_BYTECODE_TO(OPC_LSTORE_3, convert_lstore_n),
	MAP_BYTECODE_TO(OPC_FSTORE_0, convert_fstore_n),
	MAP_BYTECODE_TO(OPC_FSTORE_1, convert_fstore_n),
	MAP_BYTECODE_TO(OPC_FSTORE_2, convert_fstore_n),
	MAP_BYTECODE_TO(OPC_FSTORE_3, convert_fstore_n),
	MAP_BYTECODE_TO(OPC_DSTORE_0, convert_dstore_n),
	MAP_BYTECODE_TO(OPC_DSTORE_1, convert_dstore_n),
	MAP_BYTECODE_TO(OPC_DSTORE_2, convert_dstore_n),
	MAP_BYTECODE_TO(OPC_DSTORE_3, convert_dstore_n),
	MAP_BYTECODE_TO(OPC_ASTORE_0, convert_astore_n),
	MAP_BYTECODE_TO(OPC_ASTORE_1, convert_astore_n),
	MAP_BYTECODE_TO(OPC_ASTORE_2, convert_astore_n),
	MAP_BYTECODE_TO(OPC_ASTORE_3, convert_astore_n),
	MAP_BYTECODE_TO(OPC_IASTORE, convert_iastore),
	MAP_BYTECODE_TO(OPC_LASTORE, convert_lastore),
	MAP_BYTECODE_TO(OPC_FASTORE, convert_fastore),
	MAP_BYTECODE_TO(OPC_DASTORE, convert_dastore),
	MAP_BYTECODE_TO(OPC_AASTORE, convert_aastore),
	MAP_BYTECODE_TO(OPC_BASTORE, convert_bastore),
	MAP_BYTECODE_TO(OPC_CASTORE, convert_castore),
	MAP_BYTECODE_TO(OPC_SASTORE, convert_sastore),
	MAP_BYTECODE_TO(OPC_POP, convert_pop),
	MAP_BYTECODE_TO(OPC_POP2, convert_pop),
	MAP_BYTECODE_TO(OPC_DUP, convert_dup),
	MAP_BYTECODE_TO(OPC_DUP_X1, convert_dup_x1),
	MAP_BYTECODE_TO(OPC_DUP_X2, convert_dup_x2),
	MAP_BYTECODE_TO(OPC_DUP2, convert_dup),
	MAP_BYTECODE_TO(OPC_DUP2_X1, convert_dup_x1),
	MAP_BYTECODE_TO(OPC_DUP2_X2, convert_dup_x2),
	MAP_BYTECODE_TO(OPC_SWAP, convert_swap),
	MAP_BYTECODE_TO(OPC_IADD, convert_iadd),
	MAP_BYTECODE_TO(OPC_LADD, convert_ladd),
	MAP_BYTECODE_TO(OPC_FADD, convert_fadd),
	MAP_BYTECODE_TO(OPC_DADD, convert_dadd),
	MAP_BYTECODE_TO(OPC_ISUB, convert_isub),
	MAP_BYTECODE_TO(OPC_LSUB, convert_lsub),
	MAP_BYTECODE_TO(OPC_FSUB, convert_fsub),
	MAP_BYTECODE_TO(OPC_DSUB, convert_dsub),
	MAP_BYTECODE_TO(OPC_IMUL, convert_imul),
	MAP_BYTECODE_TO(OPC_LMUL, convert_lmul),
	MAP_BYTECODE_TO(OPC_FMUL, convert_fmul),
	MAP_BYTECODE_TO(OPC_DMUL, convert_dmul),
	MAP_BYTECODE_TO(OPC_IDIV, convert_idiv),
	MAP_BYTECODE_TO(OPC_LDIV, convert_ldiv),
	MAP_BYTECODE_TO(OPC_FDIV, convert_fdiv),
	MAP_BYTECODE_TO(OPC_DDIV, convert_ddiv),
	MAP_BYTECODE_TO(OPC_IREM, convert_irem),
	MAP_BYTECODE_TO(OPC_LREM, convert_lrem),
	MAP_BYTECODE_TO(OPC_FREM, convert_frem),
	MAP_BYTECODE_TO(OPC_DREM, convert_drem),
	MAP_BYTECODE_TO(OPC_INEG, convert_ineg),
	MAP_BYTECODE_TO(OPC_LNEG, convert_lneg),
	MAP_BYTECODE_TO(OPC_FNEG, convert_fneg),
	MAP_BYTECODE_TO(OPC_DNEG, convert_dneg),
	MAP_BYTECODE_TO(OPC_ISHL, convert_ishl),
	MAP_BYTECODE_TO(OPC_LSHL, convert_lshl),
	MAP_BYTECODE_TO(OPC_ISHR, convert_ishr),
	MAP_BYTECODE_TO(OPC_LSHR, convert_lshr),
	MAP_BYTECODE_TO(OPC_IAND, convert_iand),
	MAP_BYTECODE_TO(OPC_LAND, convert_land),
	MAP_BYTECODE_TO(OPC_IOR, convert_ior),
	MAP_BYTECODE_TO(OPC_LOR, convert_lor),
	MAP_BYTECODE_TO(OPC_IXOR, convert_ixor),
	MAP_BYTECODE_TO(OPC_LXOR, convert_lxor),
	MAP_BYTECODE_TO(OPC_IINC, convert_iinc),
	MAP_BYTECODE_TO(OPC_I2L, convert_i2l),
	MAP_BYTECODE_TO(OPC_I2F, convert_i2f),
	MAP_BYTECODE_TO(OPC_I2D, convert_i2d),
	MAP_BYTECODE_TO(OPC_L2I, convert_l2i),
	MAP_BYTECODE_TO(OPC_L2F, convert_l2f),
	MAP_BYTECODE_TO(OPC_L2D, convert_l2d),
	MAP_BYTECODE_TO(OPC_F2I, convert_f2i),
	MAP_BYTECODE_TO(OPC_F2L, convert_f2l),
	MAP_BYTECODE_TO(OPC_F2D, convert_f2d),
	MAP_BYTECODE_TO(OPC_D2I, convert_d2i),
	MAP_BYTECODE_TO(OPC_D2L, convert_d2l),
	MAP_BYTECODE_TO(OPC_D2F, convert_d2f),
	MAP_BYTECODE_TO(OPC_I2B, convert_i2b),
	MAP_BYTECODE_TO(OPC_I2C, convert_i2c),
	MAP_BYTECODE_TO(OPC_I2S, convert_i2s),
	MAP_BYTECODE_TO(OPC_LCMP, convert_lcmp),
	MAP_BYTECODE_TO(OPC_FCMPL, convert_xcmpl),
	MAP_BYTECODE_TO(OPC_FCMPG, convert_xcmpg),
	MAP_BYTECODE_TO(OPC_DCMPL, convert_xcmpl),
	MAP_BYTECODE_TO(OPC_DCMPG, convert_xcmpg),
	MAP_BYTECODE_TO(OPC_IFEQ, convert_ifeq),
	MAP_BYTECODE_TO(OPC_IFNE, convert_ifne),
	MAP_BYTECODE_TO(OPC_IFLT, convert_iflt),
	MAP_BYTECODE_TO(OPC_IFGE, convert_ifge),
	MAP_BYTECODE_TO(OPC_IFGT, convert_ifgt),
	MAP_BYTECODE_TO(OPC_IFLE, convert_ifle),
	MAP_BYTECODE_TO(OPC_IF_ICMPEQ, convert_if_icmpeq),
	MAP_BYTECODE_TO(OPC_IF_ICMPNE, convert_if_icmpne),
	MAP_BYTECODE_TO(OPC_IF_ICMPLT, convert_if_icmplt),
	MAP_BYTECODE_TO(OPC_IF_ICMPGE, convert_if_icmpge),
	MAP_BYTECODE_TO(OPC_IF_ICMPGT, convert_if_icmpgt),
	MAP_BYTECODE_TO(OPC_IF_ICMPLE, convert_if_icmple),
	MAP_BYTECODE_TO(OPC_IF_ACMPEQ, convert_if_acmpeq),
	MAP_BYTECODE_TO(OPC_IF_ACMPNE, convert_if_acmpne),
	MAP_BYTECODE_TO(OPC_GOTO, convert_goto),
	MAP_BYTECODE_TO(OPC_IRETURN, convert_non_void_return),
	MAP_BYTECODE_TO(OPC_LRETURN, convert_non_void_return),
	MAP_BYTECODE_TO(OPC_FRETURN, convert_non_void_return),
	MAP_BYTECODE_TO(OPC_DRETURN, convert_non_void_return),
	MAP_BYTECODE_TO(OPC_ARETURN, convert_non_void_return),
	MAP_BYTECODE_TO(OPC_RETURN, convert_void_return),
	MAP_BYTECODE_TO(OPC_GETSTATIC, convert_getstatic),
	MAP_BYTECODE_TO(OPC_PUTSTATIC, convert_putstatic),
	MAP_BYTECODE_TO(OPC_INVOKESTATIC, convert_invokestatic),
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
	unsigned char *code = cu->method->code;
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
