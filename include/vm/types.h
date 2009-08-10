#ifndef __VM_TYPES_H
#define __VM_TYPES_H

#include <assert.h>
#include <stdbool.h>

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

/**
 * Returns the vm type which has the same size as native pointers.
 */
static inline enum vm_type vm_pointer_type(void)
{
	if (sizeof(long) == 4)
		return J_INT;

	assert(sizeof(long) == 8);

	return J_LONG;
}

extern enum vm_type str_to_type(const char *);
extern enum vm_type get_method_return_type(char *);
extern unsigned int vm_type_size(enum vm_type);

int skip_type(const char **type);
int count_arguments(const char *);
enum vm_type bytecode_type_to_vmtype(int);
int vmtype_to_bytecode_type(enum vm_type);
int get_vmtype_size(enum vm_type);
const char *get_vm_type_name(enum vm_type);
const char *parse_method_args(const char *, enum vm_type *, char **);
unsigned int count_java_arguments(const char *);

static inline bool vm_type_is_float(enum vm_type type)
{
	return type == J_FLOAT || type == J_DOUBLE;
}

#endif
