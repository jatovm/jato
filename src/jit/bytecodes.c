/*
 * Copyright (C) 2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 */

#include <jam.h>
#include <bytecodes.h>

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
