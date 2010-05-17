#ifndef __VM_TYPES_H
#define __VM_TYPES_H

#include <assert.h>
#include <stdbool.h>
#include <string.h>

#include "lib/list.h"

struct vm_method;
struct vm_field;

enum vm_type {
	J_VOID,
	J_REFERENCE,
	J_BYTE,
	J_SHORT,
	J_INT,
	J_LONG,
	J_CHAR,
	J_FLOAT,
	J_DOUBLE,
	J_BOOLEAN,
	J_RETURN_ADDRESS,
	VM_TYPE_MAX,
};

#ifdef CONFIG_32_BIT
#  define J_NATIVE_PTR J_INT
#else
#  define J_NATIVE_PTR J_LONG
#endif

struct vm_type_info {
	enum vm_type vm_type;
	char *class_name;
};

extern enum vm_type str_to_type(const char *);
extern enum vm_type get_method_return_type(char *);
extern unsigned int vm_type_size(enum vm_type);

int skip_type(const char **type);
int count_arguments(const struct vm_method *);
enum vm_type bytecode_type_to_vmtype(int);
int vmtype_to_bytecode_type(enum vm_type);
const char *get_vm_type_name(enum vm_type);
int parse_type(char **, struct vm_type_info *);
unsigned int count_java_arguments(const struct vm_method *);
int parse_method_type(struct vm_method *);
int parse_field_type(struct vm_field *);

static inline bool vm_type_is_float(enum vm_type type)
{
	return type == J_FLOAT || type == J_DOUBLE;
}

static inline int vm_type_slot_size(enum vm_type type)
{
	if (type == J_DOUBLE || type == J_LONG)
		return 2;
	return 1;
}

static inline int vmtype_get_size(enum vm_type type)
{
	/* Currently we can load/store only multiples of machine word at once. */

	if (type == J_DOUBLE || type == J_LONG)
		return 8;

	return sizeof(unsigned long);
}

static inline bool is_primitive_array(const char *name)
{
	if (*name != '[')
		return false;

	while (*name == '[')
		name++;

	return *name != 'L';
}

static inline enum vm_type mimic_stack_type(enum vm_type type)
{
	switch (type) {
	case J_BOOLEAN:
	case J_BYTE:
	case J_CHAR:
	case J_SHORT:
		return J_INT;
	default:
		return type;
	}
}

static inline int get_arg_size(enum vm_type type)
{
	return vmtype_get_size(type) / sizeof(unsigned long);
}

#endif
