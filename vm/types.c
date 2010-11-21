#include <assert.h>

#include "vm/system.h"
#include "vm/types.h"
#include "vm/method.h"
#include "vm/field.h"
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

int count_arguments(const struct vm_method *vmm)
{
	struct vm_method_arg *arg;
	int count;

	count = 0;
	list_for_each_entry(arg, &vmm->args, list_node) {
		if (arg->type_info.vm_type == J_LONG ||
		    arg->type_info.vm_type == J_DOUBLE)
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

const char *get_vm_type_name(enum vm_type type)
{
	if (type < 0 || type >= ARRAY_SIZE(vm_type_names))
		return NULL;

	return vm_type_names[type];
}

static int parse_class_name(char **type_str)
{
	do {
		if (**type_str == 0)
			return -1;

		(*type_str)++;
	} while (**type_str != ';');

	(*type_str)++;
	return 0;
}

static int parse_array_element_type(char **type_str)
{
	switch (**type_str) {
	case '[':
		(*type_str)++;
		return parse_array_element_type(type_str);
	case 'L':
		(*type_str)++;
		return parse_class_name(type_str);
	case 'V':
	case 'B':
	case 'C':
	case 'D':
	case 'F':
	case 'I':
	case 'J':
	case 'S':
	case 'Z':
		(*type_str)++;
		return 0;
	default:
		return -1;
	}
}

int parse_type(char **type_str, struct vm_type_info *type_info)
{
	char *name_start;

	switch (**type_str) {
	case '[':
		name_start = (*type_str)++;
		if (parse_array_element_type(type_str))
			return -1;

		type_info->vm_type = J_REFERENCE;
		type_info->class_name = strndup(name_start,
			(size_t) *type_str - (size_t) name_start);
		return 0;
	case 'L':
		name_start = ++(*type_str);
		if (parse_class_name(type_str))
			return -1;

		type_info->vm_type = J_REFERENCE;
		type_info->class_name = strndup(name_start,
			(size_t) *type_str - (size_t) name_start - 1);
		return 0;
	case 'V':
	case 'B':
	case 'C':
	case 'D':
	case 'F':
	case 'I':
	case 'J':
	case 'S':
	case 'Z':
		type_info->class_name = strndup(*type_str, 1);
		type_info->vm_type = str_to_type(type_info->class_name);
		(*type_str)++;
		return 0;
	default:
		return -1;
	}
}

static struct vm_method_arg *alloc_method_arg(void)
{
	struct vm_method_arg *arg;

	arg = malloc(sizeof(*arg));
	if (!arg)
		return NULL;

	INIT_LIST_HEAD(&arg->list_node);
	return arg;
}

int parse_method_type(struct vm_method *vmm)
{
	char *type_str;

	INIT_LIST_HEAD(&vmm->args);
	type_str = vmm->type;

	if (*type_str++ != '(')
		return -1;

	while (*type_str != ')') {
		struct vm_method_arg *arg = alloc_method_arg();
		if (!arg)
			return -ENOMEM;

		if (parse_type(&type_str, &arg->type_info))
			return -1;

		list_add_tail(&arg->list_node, &vmm->args);
	}

	type_str++;

	/* parse return type */
	if (parse_type(&type_str, &vmm->return_type))
		return -1;

	if (*type_str != 0)
		return -1; /* junk after return type */

	return 0;
}

int parse_field_type(struct vm_field *vmf)
{
	char *type_str;

	type_str = vmf->type;
	return parse_type(&type_str, &vmf->type_info);
}

unsigned int vm_method_arg_slots(const struct vm_method *vmm)
{
	struct vm_method_arg *arg;
	unsigned int count;

	count = 0;
	list_for_each_entry(arg, &vmm->args, list_node) {
		count++;

		if (vm_type_is_pair(arg->type_info.vm_type))
			count++;
	}

	return count;
}

unsigned int count_java_arguments(const struct vm_method *vmm)
{
	struct vm_method_arg *arg;
	unsigned int count;

	count = 0;
	list_for_each_entry(arg, &vmm->args, list_node) {
		count++;
	}

	return count;
}
