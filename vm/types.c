#include <vm/types.h>

/* See Table 4.2 in Section 4.3.2 ("Field Descriptors") of the JVM
   specification.  */
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
	return J_REFERENCE;	/* L<classname>; or [ */
}

enum vm_type get_method_return_type(char *type)
{
	while (*type != ')')
		type++;

	return str_to_type(type + 1);
}

#include <stdio.h>

static unsigned int count_skip_arguments(const char **type);

static int skip_type(const char **type)
{
	const char *ptr = *type;

	switch (*ptr) {
	/* BaseType */
	case 'B':
	case 'C':
	case 'D':
	case 'F':
	case 'I':
	case 'J':
	case 'S':
	case 'Z':
		++ptr;
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
		if (skip_type(&ptr))
			return -1;
		break;

	default:
		NOT_IMPLEMENTED;
		return -1;
	}

	*type = ptr;
	return 0;
}

static unsigned int count_skip_arguments(const char **type)
{
	unsigned int args = 0;
	const char *ptr = *type;

	while (1) {
		if (!*ptr)
			break;

		if (*ptr == ')')
			break;

		if (skip_type(&ptr))
			return -1;

		++args;
	}

	*type = ptr;
	return args;
}

unsigned int count_arguments(const char *type)
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
