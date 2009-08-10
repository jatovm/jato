#include <assert.h>

#include "vm/system.h"
#include "vm/types.h"
#include "vm/die.h"
#include "vm/vm.h"

#include "jit/args.h"

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

unsigned int vm_type_size(enum vm_type type)
{
	static const int size[VM_TYPE_MAX] = {
		[J_VOID] = 0,
		[J_REFERENCE] = sizeof(void *),
		[J_BYTE] = 1,
		[J_SHORT] = 2,
		[J_INT] = 4,
		[J_LONG] = 8,
		[J_CHAR] = 2,
		[J_FLOAT] = 4,
		[J_DOUBLE] = 8,
		[J_BOOLEAN] = 1,
		[J_RETURN_ADDRESS] = sizeof(void *),
	};

	return size[type];
}

int count_arguments(const char *type)
{
	enum vm_type vmtype;
	int count;

	count = 0;

	while ((type = parse_method_args(type, &vmtype, NULL))) {
		if (vmtype == J_LONG || vmtype == J_DOUBLE)
			count += 2;
		else
			count++;
	}

	return count;
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
	/* Note: The cast below is okay, because we _know_ that type is non-
	 * negative at that point. */
	assert(type >= 0);
	assert((unsigned int) type < ARRAY_SIZE(bytecode_type_to_vmtype_map));

	return bytecode_type_to_vmtype_map[type];
}

static int vmtype_to_bytecode_type_map[] = {
	[J_BOOLEAN] = T_BOOLEAN,
	[J_CHAR] = T_CHAR,
	[J_FLOAT] = T_FLOAT,
	[J_DOUBLE] = T_DOUBLE,
	[J_BYTE] = T_BYTE,
	[J_SHORT] = T_SHORT,
	[J_INT] = T_INT,
	[J_LONG] = T_LONG,
};

int vmtype_to_bytecode_type(enum vm_type type)
{
	assert(type >= 0 && type < ARRAY_SIZE(vmtype_to_bytecode_type_map));

	return vmtype_to_bytecode_type_map[type];
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

const char *parse_method_args(const char *type_str, enum vm_type *vmtype,
			      char **name_p)
{
	const char *type_name_start;

	if (*type_str == '(')
		type_str++;

	type_name_start = type_str;

	if (name_p)
		*name_p = NULL;

	if (*type_str == ')')
		return NULL;

	if (*type_str == '[') {
		*vmtype = J_REFERENCE;
		type_str++;

		if (*type_str != 'L') {
			type_str++;
			goto out;
		}
	}

	if (*type_str == 'L') {
		++type_name_start;
		++type_str;
		while (*(type_str++) != ';')
			;
		*vmtype = J_REFERENCE;
	} else {
		char primitive_name[2];

		primitive_name[0] = *(type_str++);
		primitive_name[1] = 0;

		*vmtype = str_to_type(primitive_name);
	}

 out:
	if (name_p) {
		size_t size = (size_t) type_str - (size_t) type_name_start;

		if (*vmtype == J_REFERENCE)
			size--;

		*name_p = strndup(type_name_start, size);
	}

	return type_str;
}

unsigned int count_java_arguments(const char *type)
{
	unsigned int count;
	enum vm_type vmtype;

	count = 0;

	while ((type = parse_method_args(type, &vmtype, NULL)))
		count ++;

	return count;
}
