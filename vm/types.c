#include "vm/system.h"
#include "vm/types.h"
#include "vm/die.h"
#include "vm/vm.h"

/* See Table 4.2 in Section 4.3.2 ("Field Descriptors") of the JVM
   specification.  */
enum vm_type str_to_type(const char *type)
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
	return J_REFERENCE;	/* L<classname>; or [ */
}

enum vm_type get_method_return_type(char *type)
{
	while (*type != ')')
		type++;

	return str_to_type(type + 1);
}

#include <stdio.h>

static int count_skip_arguments(const char **type);

static int skip_type(const char **type)
{
	const char *ptr = *type;
	int ret = 1;

	switch (*ptr) {
		/* BaseType */
	case 'B':
	case 'C':
	case 'F':
	case 'I':
	case 'S':
	case 'Z':
		++ptr;
		break;

	case 'D':
	case 'J':
		++ptr;
		ret = 2;
		break;

		/* ObjectType */
	case 'L':
		while (1) {
			if (!*ptr)
				return -1;
			if (*ptr == ';')
				break;

			++ptr;
		}

		++ptr;
		break;

		/* ArrayType */
	case '[':
		++ptr;
		ret = skip_type(&ptr);
		break;

	default:
		NOT_IMPLEMENTED;
		return -1;
	}

	*type = ptr;
	return ret;
}

static int count_skip_arguments(const char **type)
{
	unsigned int args = 0;
	const char *ptr = *type;

	while (1) {
		int ret;

		if (!*ptr)
			break;

		if (*ptr == ')')
			break;

		ret = skip_type(&ptr);
		if (ret < 0)
			return -1;

		args += ret;
	}

	*type = ptr;
	return args;
}

int count_arguments(const char *type)
{
	unsigned int args = 0;

	if (*type != '(')
		return -1;
	++type;

	args = count_skip_arguments(&type);

	if (*type != ')')
		return -1;

	return args;
}

static enum vm_type bytecode_type_to_vmtype_map[] = {
	[T_BOOLEAN] = J_BOOLEAN,
	[T_CHAR] = J_CHAR,
	[T_FLOAT] = J_FLOAT,
	[T_DOUBLE] = J_DOUBLE,
	[T_BYTE] = J_BYTE,
	[T_SHORT] = J_SHORT,
	[T_INT] = J_INT,
	[T_LONG] = J_LONG,
};

enum vm_type bytecode_type_to_vmtype(int type)
{
	return bytecode_type_to_vmtype_map[type];
}

int get_vmtype_size(enum vm_type type)
{
	/* Currently we can load/store only multiples of machine word
	   at once. */
	switch (type) {
	case J_BOOLEAN:
	case J_BYTE:
	case J_CHAR:
	case J_SHORT:
	case J_FLOAT:
	case J_INT:
	case J_REFERENCE:
		return sizeof(unsigned long);
	case J_DOUBLE:
	case J_LONG:
		return 8;
	default:
		error("type has no size");
	}
}

static const char *vm_type_names[] = {
	[J_VOID] = "J_VOID",
	[J_REFERENCE] = "J_REFERENCE",
	[J_BYTE] = "J_BYTE",
	[J_SHORT] = "J_SHORT",
	[J_INT] = "J_INT",
	[J_LONG] = "J_LONG",
	[J_CHAR] = "J_CHAR",
	[J_FLOAT] = "J_FLOAT",
	[J_DOUBLE] = "J_DOUBLE",
	[J_BOOLEAN] = "J_BOOLEAN",
	[J_RETURN_ADDRESS] = "J_RETURN_ADDRESS"
};

const char *get_vm_type_name(enum vm_type type) {
	if (type < 0 || type >= ARRAY_SIZE(vm_type_names))
		return NULL;

	return vm_type_names[type];
}
