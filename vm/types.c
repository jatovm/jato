#include <vm/types.h>

enum vm_type str_to_type(char *type)
{
	switch (type[0]) {
	case 'V':
		return J_VOID;
	case 'B':
		return J_BYTE;
	case 'C':
		return J_CHAR;
	case 'D':
		return J_DOUBLE;
	case 'F':
		return J_FLOAT;
	case 'I':
		return J_INT;
	case 'J':
		return J_LONG;
	case 'S':
		return J_SHORT;
	case 'Z':
		return J_BOOLEAN;
	default:
		break;
	};
	return J_REFERENCE;
}
