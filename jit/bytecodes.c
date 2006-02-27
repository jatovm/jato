/*
 * Copyright (C) 2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 *
 * The file contains utility functions for parsing a bytecode stream.
 */

#include <jam.h>
#include <bytecodes.h>
#include <byteorder.h>

static unsigned char bytecode_sizes[];

unsigned long bytecode_size(unsigned char *bc_start)
{
	if (*bc_start == OPC_WIDE)
		return bytecode_sizes[OPC_WIDE] + bytecode_sizes[*++bc_start];

	return bytecode_sizes[*bc_start];
}

bool bytecode_is_branch(unsigned char opc)
{
	bool branch;
	switch (opc) {
		case OPC_IFEQ:
		case OPC_IFNE:
		case OPC_IFLT:
		case OPC_IFGE:
		case OPC_IFGT:
		case OPC_IFLE:
		case OPC_IF_ICMPEQ:
		case OPC_IF_ICMPNE:
		case OPC_IF_ICMPLT:
		case OPC_IF_ICMPGE:
		case OPC_IF_ICMPGT:
		case OPC_IF_ICMPLE:
		case OPC_IF_ACMPEQ:
		case OPC_IF_ACMPNE:
		case OPC_GOTO:
		case OPC_JSR:
		case OPC_TABLESWITCH:
		case OPC_LOOKUPSWITCH:
		case OPC_IFNULL:
		case OPC_IFNONNULL:
		case OPC_GOTO_W:
		case OPC_JSR_W:
			branch = true;
			break;
		default:
			branch = false;
	};
	return branch;
}

static unsigned long bytecode_br_target16(unsigned char* code)
{
	u2 target = *(u2 *) code;
	return be16_to_cpu(target);
}

static unsigned long bytecode_br_target32(unsigned char* code)
{
	u4 target = *(u4 *) code;
	return be32_to_cpu(target);
}

/**
 *	bytecode_br_target - Return branch opcode target offset.
 *	@code: start of branch bytecode.
 */
unsigned long bytecode_br_target(unsigned char *code)
{
	unsigned char opc = *code;

	if (opc == OPC_GOTO_W || opc == OPC_JSR_W)
		return bytecode_br_target32(code + 1);

	return bytecode_br_target16(code + 1);
}

static unsigned char bytecode_sizes[] = {
	[OPC_NOP] = 1,
	[OPC_ACONST_NULL] = 1,
	[OPC_ICONST_M1] = 1,
	[OPC_ICONST_0] = 1,
	[OPC_ICONST_1] = 1,
	[OPC_ICONST_2] = 1,
	[OPC_ICONST_3] = 1,
	[OPC_ICONST_4] = 1,
	[OPC_ICONST_5] = 1,
	[OPC_LCONST_0] = 1,
	[OPC_LCONST_1] = 1,
	[OPC_FCONST_0] = 1,
	[OPC_FCONST_1] = 1,
	[OPC_FCONST_2] = 1,
	[OPC_DCONST_0] = 1,
	[OPC_DCONST_1] = 1,
	[OPC_BIPUSH] = 2,
	[OPC_SIPUSH] = 3,
	[OPC_LDC] = 2,
	[OPC_LDC_W] = 3,
	[OPC_LDC2_W] = 3,
	[OPC_ILOAD] = 2,
	[OPC_LLOAD] = 2,
	[OPC_FLOAD] = 2,
	[OPC_DLOAD] = 2,
	[OPC_ALOAD] = 2,
	[OPC_ILOAD_0] = 1,
	[OPC_ILOAD_1] = 1,
	[OPC_ILOAD_2] = 1,
	[OPC_ILOAD_3] = 1,
	[OPC_LLOAD_0] = 1,
	[OPC_LLOAD_1] = 1,
	[OPC_LLOAD_2] = 1,
	[OPC_LLOAD_3] = 1,
	[OPC_FLOAD_0] = 1,
	[OPC_FLOAD_1] = 1,
	[OPC_FLOAD_2] = 1,
	[OPC_FLOAD_3] = 1,
	[OPC_DLOAD_0] = 1,
	[OPC_DLOAD_1] = 1,
	[OPC_DLOAD_2] = 1,
	[OPC_DLOAD_3] = 1,
	[OPC_ALOAD_0] = 1,
	[OPC_ALOAD_1] = 1,
	[OPC_ALOAD_2] = 1,
	[OPC_ALOAD_3] = 1,
	[OPC_IALOAD] = 1,
	[OPC_LALOAD] = 1,
	[OPC_FALOAD] = 1,
	[OPC_DALOAD] = 1,
	[OPC_AALOAD] = 1,
	[OPC_BALOAD] = 1,
	[OPC_CALOAD] = 1,
	[OPC_SALOAD] = 1,
	[OPC_ISTORE] = 2,
	[OPC_LSTORE] = 2,
	[OPC_FSTORE] = 2,
	[OPC_DSTORE] = 2,
	[OPC_ASTORE] = 2,
	[OPC_ISTORE_0] = 1,
	[OPC_ISTORE_1] = 1,
	[OPC_ISTORE_2] = 1,
	[OPC_ISTORE_3] = 1,
	[OPC_LSTORE_0] = 1,
	[OPC_LSTORE_1] = 1,
	[OPC_LSTORE_2] = 1,
	[OPC_LSTORE_3] = 1,
	[OPC_FSTORE_0] = 1,
	[OPC_FSTORE_1] = 1,
	[OPC_FSTORE_2] = 1,
	[OPC_FSTORE_3] = 1,
	[OPC_DSTORE_0] = 1,
	[OPC_DSTORE_1] = 1,
	[OPC_DSTORE_2] = 1,
	[OPC_DSTORE_3] = 1,
	[OPC_ASTORE_0] = 1,
	[OPC_ASTORE_1] = 1,
	[OPC_ASTORE_2] = 1,
	[OPC_ASTORE_3] = 1,
	[OPC_IASTORE] = 1,
	[OPC_LASTORE] = 1,
	[OPC_FASTORE] = 1,
	[OPC_DASTORE] = 1,
	[OPC_AASTORE] = 1,
	[OPC_BASTORE] = 1,
	[OPC_CASTORE] = 1,
	[OPC_SASTORE] = 1,
	[OPC_POP] = 1,
	[OPC_POP2] = 1,
	[OPC_DUP] = 1,
	[OPC_DUP_X1] = 1,
	[OPC_DUP_X2] = 1,
	[OPC_DUP2] = 1,
	[OPC_DUP2_X1] = 1,
	[OPC_DUP2_X2] = 1,
	[OPC_SWAP] = 1,
	[OPC_IADD] = 1,
	[OPC_LADD] = 1,
	[OPC_FADD] = 1,
	[OPC_DADD] = 1,
	[OPC_ISUB] = 1,
	[OPC_LSUB] = 1,
	[OPC_FSUB] = 1,
	[OPC_DSUB] = 1,
	[OPC_IMUL] = 1,
	[OPC_LMUL] = 1,
	[OPC_FMUL] = 1,
	[OPC_DMUL] = 1,
	[OPC_IDIV] = 1,
	[OPC_LDIV] = 1,
	[OPC_FDIV] = 1,
	[OPC_DDIV] = 1,
	[OPC_IREM] = 1,
	[OPC_LREM] = 1,
	[OPC_FREM] = 1,
	[OPC_DREM] = 1,
	[OPC_INEG] = 1,
	[OPC_LNEG] = 1,
	[OPC_FNEG] = 1,
	[OPC_DNEG] = 1,
	[OPC_ISHL] = 1,
	[OPC_LSHL] = 1,
	[OPC_ISHR] = 1,
	[OPC_LSHR] = 1,
	[OPC_IAND] = 1,
	[OPC_LAND] = 1,
	[OPC_IOR] = 1,
	[OPC_LOR] = 1,
	[OPC_IXOR] = 1,
	[OPC_LXOR] = 1,
	[OPC_IINC] = 3,
	[OPC_I2L] = 1,
	[OPC_I2F] = 1,
	[OPC_I2D] = 1,
	[OPC_L2I] = 1,
	[OPC_L2F] = 1,
	[OPC_L2D] = 1,
	[OPC_F2I] = 1,
	[OPC_F2L] = 1,
	[OPC_F2D] = 1,
	[OPC_D2I] = 1,
	[OPC_D2L] = 1,
	[OPC_D2F] = 1,
	[OPC_I2B] = 1,
	[OPC_I2C] = 1,
	[OPC_I2S] = 1,
	[OPC_LCMP] = 1,
	[OPC_FCMPL] = 1,
	[OPC_FCMPG] = 1,
	[OPC_DCMPL] = 1,
	[OPC_DCMPG] = 1,
	[OPC_IFEQ] = 3,
	[OPC_IFNE] = 3,
	[OPC_IFLT] = 3,
	[OPC_IFGE] = 3,
	[OPC_IFGT] = 3,
	[OPC_IFLE] = 3,
	[OPC_IF_ICMPEQ] = 3,
	[OPC_IF_ICMPNE] = 3,
	[OPC_IF_ICMPLT] = 3,
	[OPC_IF_ICMPGE] = 3,
	[OPC_IF_ICMPGT] = 3,
	[OPC_IF_ICMPLE] = 3,
	[OPC_IF_ACMPEQ] = 3,
	[OPC_IF_ACMPNE] = 3,
	[OPC_GOTO] = 3,
	[OPC_JSR] = 3,
	[OPC_RET] = 2,
	/* OPC_TABLESWITCH and OPC_LOOKUPSWITCH are variable-length.  */
	[OPC_IRETURN] = 1,
	[OPC_LRETURN] = 1,
	[OPC_FRETURN] = 1,
	[OPC_DRETURN] = 1,
	[OPC_ARETURN] = 1,
	[OPC_RETURN] = 1,
	[OPC_GETSTATIC] = 3,
	[OPC_PUTSTATIC] = 3,
	[OPC_GETFIELD] = 3,
	[OPC_PUTFIELD] = 3,
	[OPC_WIDE] = 3,
	[OPC_IFNONNULL] = 3,
	[OPC_IRETURN] = 1,
	[OPC_LRETURN] = 1,
	[OPC_FRETURN] = 1,
	[OPC_DRETURN] = 1,
	[OPC_ARETURN] = 1,
	[OPC_RETURN] = 1,
};
