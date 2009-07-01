#ifndef __VM_TYPES_H
#define __VM_TYPES_H

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

extern enum vm_type str_to_type(const char *);
extern enum vm_type get_method_return_type(char *);

int count_arguments(const char *);
enum vm_type bytecode_type_to_vmtype(int);
int get_vmtype_size(enum vm_type);

#endif
