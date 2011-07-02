/*
 * Copyright (c) 2011 Pekka Enberg
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

#include "vm/interp.h"

#include "cafebabe/code_attribute.h"
#include "vm/method.h"

#include <assert.h>
#include <stdio.h>

enum interp_status {
	INTERP_CONTINUE,
	INTERP_RETURN,
};

static enum interp_status interp(uint8_t opc)
{
	switch (opc) {
	case OPC_NOP:
		 break;
	case OPC_ACONST_NULL:	assert(!"OPC_ACONST_NULL"); break;
	case OPC_ICONST_M1:	assert(!"OPC_ICONST_M1"); break;
	case OPC_ICONST_0:	assert(!"OPC_ICONST_0"); break;
	case OPC_ICONST_1:	assert(!"OPC_ICONST_1"); break;
	case OPC_ICONST_2:	assert(!"OPC_ICONST_2"); break;
	case OPC_ICONST_3:	assert(!"OPC_ICONST_3"); break;
	case OPC_ICONST_4:	assert(!"OPC_ICONST_4"); break;
	case OPC_ICONST_5:	assert(!"OPC_ICONST_5"); break;
	case OPC_LCONST_0:	assert(!"OPC_LCONST_0"); break;
	case OPC_LCONST_1:	assert(!"OPC_LCONST_1"); break;
	case OPC_FCONST_0:	assert(!"OPC_FCONST_0"); break;
	case OPC_FCONST_1:	assert(!"OPC_FCONST_1"); break;
	case OPC_FCONST_2:	assert(!"OPC_FCONST_2"); break;
	case OPC_DCONST_0:	assert(!"OPC_DCONST_0"); break;
	case OPC_DCONST_1:	assert(!"OPC_DCONST_1"); break;
	case OPC_BIPUSH:	assert(!"OPC_BIPUSH"); break;
	case OPC_SIPUSH:	assert(!"OPC_SIPUSH"); break;
	case OPC_LDC:	assert(!"OPC_LDC"); break;
	case OPC_LDC_W:	assert(!"OPC_LDC_W"); break;
	case OPC_LDC2_W:	assert(!"OPC_LDC2_W"); break;
	case OPC_ILOAD:	assert(!"OPC_ILOAD"); break;
	case OPC_LLOAD:	assert(!"OPC_LLOAD"); break;
	case OPC_FLOAD:	assert(!"OPC_FLOAD"); break;
	case OPC_DLOAD:	assert(!"OPC_DLOAD"); break;
	case OPC_ALOAD:	assert(!"OPC_ALOAD"); break;
	case OPC_ILOAD_0:	assert(!"OPC_ILOAD_0"); break;
	case OPC_ILOAD_1:	assert(!"OPC_ILOAD_1"); break;
	case OPC_ILOAD_2:	assert(!"OPC_ILOAD_2"); break;
	case OPC_ILOAD_3:	assert(!"OPC_ILOAD_3"); break;
	case OPC_LLOAD_0:	assert(!"OPC_LLOAD_0"); break;
	case OPC_LLOAD_1:	assert(!"OPC_LLOAD_1"); break;
	case OPC_LLOAD_2:	assert(!"OPC_LLOAD_2"); break;
	case OPC_LLOAD_3:	assert(!"OPC_LLOAD_3"); break;
	case OPC_FLOAD_0:	assert(!"OPC_FLOAD_0"); break;
	case OPC_FLOAD_1:	assert(!"OPC_FLOAD_1"); break;
	case OPC_FLOAD_2:	assert(!"OPC_FLOAD_2"); break;
	case OPC_FLOAD_3:	assert(!"OPC_FLOAD_3"); break;
	case OPC_DLOAD_0:	assert(!"OPC_DLOAD_0"); break;
	case OPC_DLOAD_1:	assert(!"OPC_DLOAD_1"); break;
	case OPC_DLOAD_2:	assert(!"OPC_DLOAD_2"); break;
	case OPC_DLOAD_3:	assert(!"OPC_DLOAD_3"); break;
	case OPC_ALOAD_0:	assert(!"OPC_ALOAD_0"); break;
	case OPC_ALOAD_1:	assert(!"OPC_ALOAD_1"); break;
	case OPC_ALOAD_2:	assert(!"OPC_ALOAD_2"); break;
	case OPC_ALOAD_3:	assert(!"OPC_ALOAD_3"); break;
	case OPC_IALOAD:	assert(!"OPC_IALOAD"); break;
	case OPC_LALOAD:	assert(!"OPC_LALOAD"); break;
	case OPC_FALOAD:	assert(!"OPC_FALOAD"); break;
	case OPC_DALOAD:	assert(!"OPC_DALOAD"); break;
	case OPC_AALOAD:	assert(!"OPC_AALOAD"); break;
	case OPC_BALOAD:	assert(!"OPC_BALOAD"); break;
	case OPC_CALOAD:	assert(!"OPC_CALOAD"); break;
	case OPC_SALOAD:	assert(!"OPC_SALOAD"); break;
	case OPC_ISTORE:	assert(!"OPC_ISTORE"); break;
	case OPC_LSTORE:	assert(!"OPC_LSTORE"); break;
	case OPC_FSTORE:	assert(!"OPC_FSTORE"); break;
	case OPC_DSTORE:	assert(!"OPC_DSTORE"); break;
	case OPC_ASTORE:	assert(!"OPC_ASTORE"); break;
	case OPC_ISTORE_0:	assert(!"OPC_ISTORE_0"); break;
	case OPC_ISTORE_1:	assert(!"OPC_ISTORE_1"); break;
	case OPC_ISTORE_2:	assert(!"OPC_ISTORE_2"); break;
	case OPC_ISTORE_3:	assert(!"OPC_ISTORE_3"); break;
	case OPC_LSTORE_0:	assert(!"OPC_LSTORE_0"); break;
	case OPC_LSTORE_1:	assert(!"OPC_LSTORE_1"); break;
	case OPC_LSTORE_2:	assert(!"OPC_LSTORE_2"); break;
	case OPC_LSTORE_3:	assert(!"OPC_LSTORE_3"); break;
	case OPC_FSTORE_0:	assert(!"OPC_FSTORE_0"); break;
	case OPC_FSTORE_1:	assert(!"OPC_FSTORE_1"); break;
	case OPC_FSTORE_2:	assert(!"OPC_FSTORE_2"); break;
	case OPC_FSTORE_3:	assert(!"OPC_FSTORE_3"); break;
	case OPC_DSTORE_0:	assert(!"OPC_DSTORE_0"); break;
	case OPC_DSTORE_1:	assert(!"OPC_DSTORE_1"); break;
	case OPC_DSTORE_2:	assert(!"OPC_DSTORE_2"); break;
	case OPC_DSTORE_3:	assert(!"OPC_DSTORE_3"); break;
	case OPC_ASTORE_0:	assert(!"OPC_ASTORE_0"); break;
	case OPC_ASTORE_1:	assert(!"OPC_ASTORE_1"); break;
	case OPC_ASTORE_2:	assert(!"OPC_ASTORE_2"); break;
	case OPC_ASTORE_3:	assert(!"OPC_ASTORE_3"); break;
	case OPC_IASTORE:	assert(!"OPC_IASTORE"); break;
	case OPC_LASTORE:	assert(!"OPC_LASTORE"); break;
	case OPC_FASTORE:	assert(!"OPC_FASTORE"); break;
	case OPC_DASTORE:	assert(!"OPC_DASTORE"); break;
	case OPC_AASTORE:	assert(!"OPC_AASTORE"); break;
	case OPC_BASTORE:	assert(!"OPC_BASTORE"); break;
	case OPC_CASTORE:	assert(!"OPC_CASTORE"); break;
	case OPC_SASTORE:	assert(!"OPC_SASTORE"); break;
	case OPC_POP:	assert(!"OPC_POP"); break;
	case OPC_POP2:	assert(!"OPC_POP2"); break;
	case OPC_DUP:	assert(!"OPC_DUP"); break;
	case OPC_DUP_X1:	assert(!"OPC_DUP_X1"); break;
	case OPC_DUP_X2:	assert(!"OPC_DUP_X2"); break;
	case OPC_DUP2:	assert(!"OPC_DUP2"); break;
	case OPC_DUP2_X1:	assert(!"OPC_DUP2_X1"); break;
	case OPC_DUP2_X2:	assert(!"OPC_DUP2_X2"); break;
	case OPC_SWAP:	assert(!"OPC_SWAP"); break;
	case OPC_IADD:	assert(!"OPC_IADD"); break;
	case OPC_LADD:	assert(!"OPC_LADD"); break;
	case OPC_FADD:	assert(!"OPC_FADD"); break;
	case OPC_DADD:	assert(!"OPC_DADD"); break;
	case OPC_ISUB:	assert(!"OPC_ISUB"); break;
	case OPC_LSUB:	assert(!"OPC_LSUB"); break;
	case OPC_FSUB:	assert(!"OPC_FSUB"); break;
	case OPC_DSUB:	assert(!"OPC_DSUB"); break;
	case OPC_IMUL:	assert(!"OPC_IMUL"); break;
	case OPC_LMUL:	assert(!"OPC_LMUL"); break;
	case OPC_FMUL:	assert(!"OPC_FMUL"); break;
	case OPC_DMUL:	assert(!"OPC_DMUL"); break;
	case OPC_IDIV:	assert(!"OPC_IDIV"); break;
	case OPC_LDIV:	assert(!"OPC_LDIV"); break;
	case OPC_FDIV:	assert(!"OPC_FDIV"); break;
	case OPC_DDIV:	assert(!"OPC_DDIV"); break;
	case OPC_IREM:	assert(!"OPC_IREM"); break;
	case OPC_LREM:	assert(!"OPC_LREM"); break;
	case OPC_FREM:	assert(!"OPC_FREM"); break;
	case OPC_DREM:	assert(!"OPC_DREM"); break;
	case OPC_INEG:	assert(!"OPC_INEG"); break;
	case OPC_LNEG:	assert(!"OPC_LNEG"); break;
	case OPC_FNEG:	assert(!"OPC_FNEG"); break;
	case OPC_DNEG:	assert(!"OPC_DNEG"); break;
	case OPC_ISHL:	assert(!"OPC_ISHL"); break;
	case OPC_LSHL:	assert(!"OPC_LSHL"); break;
	case OPC_ISHR:	assert(!"OPC_ISHR"); break;
	case OPC_LSHR:	assert(!"OPC_LSHR"); break;
	case OPC_IUSHR:	assert(!"OPC_IUSHR"); break;
	case OPC_LUSHR:	assert(!"OPC_LUSHR"); break;
	case OPC_IAND:	assert(!"OPC_IAND"); break;
	case OPC_LAND:	assert(!"OPC_LAND"); break;
	case OPC_IOR:	assert(!"OPC_IOR"); break;
	case OPC_LOR:	assert(!"OPC_LOR"); break;
	case OPC_IXOR:	assert(!"OPC_IXOR"); break;
	case OPC_LXOR:	assert(!"OPC_LXOR"); break;
	case OPC_IINC:	assert(!"OPC_IINC"); break;
	case OPC_I2L:	assert(!"OPC_I2L"); break;
	case OPC_I2F:	assert(!"OPC_I2F"); break;
	case OPC_I2D:	assert(!"OPC_I2D"); break;
	case OPC_L2I:	assert(!"OPC_L2I"); break;
	case OPC_L2F:	assert(!"OPC_L2F"); break;
	case OPC_L2D:	assert(!"OPC_L2D"); break;
	case OPC_F2I:	assert(!"OPC_F2I"); break;
	case OPC_F2L:	assert(!"OPC_F2L"); break;
	case OPC_F2D:	assert(!"OPC_F2D"); break;
	case OPC_D2I:	assert(!"OPC_D2I"); break;
	case OPC_D2L:	assert(!"OPC_D2L"); break;
	case OPC_D2F:	assert(!"OPC_D2F"); break;
	case OPC_I2B:	assert(!"OPC_I2B"); break;
	case OPC_I2C:	assert(!"OPC_I2C"); break;
	case OPC_I2S:	assert(!"OPC_I2S"); break;
	case OPC_LCMP:	assert(!"OPC_LCMP"); break;
	case OPC_FCMPL:	assert(!"OPC_FCMPL"); break;
	case OPC_FCMPG:	assert(!"OPC_FCMPG"); break;
	case OPC_DCMPL:	assert(!"OPC_DCMPL"); break;
	case OPC_DCMPG:	assert(!"OPC_DCMPG"); break;
	case OPC_IFEQ:	assert(!"OPC_IFEQ"); break;
	case OPC_IFNE:	assert(!"OPC_IFNE"); break;
	case OPC_IFLT:	assert(!"OPC_IFLT"); break;
	case OPC_IFGE:	assert(!"OPC_IFGE"); break;
	case OPC_IFGT:	assert(!"OPC_IFGT"); break;
	case OPC_IFLE:	assert(!"OPC_IFLE"); break;
	case OPC_IF_ICMPEQ:	assert(!"OPC_IF_ICMPEQ"); break;
	case OPC_IF_ICMPNE:	assert(!"OPC_IF_ICMPNE"); break;
	case OPC_IF_ICMPLT:	assert(!"OPC_IF_ICMPLT"); break;
	case OPC_IF_ICMPGE:	assert(!"OPC_IF_ICMPGE"); break;
	case OPC_IF_ICMPGT:	assert(!"OPC_IF_ICMPGT"); break;
	case OPC_IF_ICMPLE:	assert(!"OPC_IF_ICMPLE"); break;
	case OPC_IF_ACMPEQ:	assert(!"OPC_IF_ACMPEQ"); break;
	case OPC_IF_ACMPNE:	assert(!"OPC_IF_ACMPNE"); break;
	case OPC_GOTO:	assert(!"OPC_GOTO"); break;
	case OPC_JSR:	assert(!"OPC_JSR"); break;
	case OPC_RET:	assert(!"OPC_RET"); break;
	case OPC_TABLESWITCH:	assert(!"OPC_TABLESWITCH"); break;
	case OPC_LOOKUPSWITCH:	assert(!"OPC_LOOKUPSWITCH"); break;
	case OPC_IRETURN:	assert(!"OPC_IRETURN"); break;
	case OPC_LRETURN:	assert(!"OPC_LRETURN"); break;
	case OPC_FRETURN:	assert(!"OPC_FRETURN"); break;
	case OPC_DRETURN:	assert(!"OPC_DRETURN"); break;
	case OPC_ARETURN:	assert(!"OPC_ARETURN"); break;
	case OPC_RETURN:
		return INTERP_RETURN;
	case OPC_GETSTATIC:	assert(!"OPC_GETSTATIC"); break;
	case OPC_PUTSTATIC:	assert(!"OPC_PUTSTATIC"); break;
	case OPC_GETFIELD:	assert(!"OPC_GETFIELD"); break;
	case OPC_PUTFIELD:	assert(!"OPC_PUTFIELD"); break;
	case OPC_INVOKEVIRTUAL:	assert(!"OPC_INVOKEVIRTUAL"); break;
	case OPC_INVOKESPECIAL:	assert(!"OPC_INVOKESPECIAL"); break;
	case OPC_INVOKESTATIC:	assert(!"OPC_INVOKESTATIC"); break;
	case OPC_INVOKEINTERFACE:	assert(!"OPC_INVOKEINTERFACE"); break;
	case OPC_NEW:	assert(!"OPC_NEW"); break;
	case OPC_NEWARRAY:	assert(!"OPC_NEWARRAY"); break;
	case OPC_ANEWARRAY:	assert(!"OPC_ANEWARRAY"); break;
	case OPC_ARRAYLENGTH:	assert(!"OPC_ARRAYLENGTH"); break;
	case OPC_ATHROW:	assert(!"OPC_ATHROW"); break;
	case OPC_CHECKCAST:	assert(!"OPC_CHECKCAST"); break;
	case OPC_INSTANCEOF:	assert(!"OPC_INSTANCEOF"); break;
	case OPC_MONITORENTER:	assert(!"OPC_MONITORENTER"); break;
	case OPC_MONITOREXIT:	assert(!"OPC_MONITOREXIT"); break;
	case OPC_WIDE:	assert(!"OPC_WIDE"); break;
	case OPC_MULTIANEWARRAY:	assert(!"OPC_MULTIANEWARRAY"); break;
	case OPC_IFNULL:	assert(!"OPC_IFNULL"); break;
	case OPC_IFNONNULL:	assert(!"OPC_IFNONNULL"); break;
	case OPC_GOTO_W:	assert(!"OPC_GOTO_W"); break;
	case OPC_JSR_W:	assert(!"OPC_JSR_W"); break;
	case OPC_BREAKPOINT:	assert(!"OPC_BREAKPOINT"); break;
	default:
		assert(!"unknown bytecode");
	}

	return INTERP_CONTINUE;
}

void vm_interp_method_v(struct vm_method *method, va_list args, union jvalue *result)
{
	uint32_t pc;

	pc = 0;
	while (pc < method->code_attribute.code_length) {
		uint8_t opc = method->code_attribute.code[pc];

		if (interp(opc) == INTERP_RETURN)
			return;

		pc++;
	}
}
