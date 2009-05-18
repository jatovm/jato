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
};

extern enum vm_type str_to_type(char *);
extern enum vm_type get_method_return_type(char *);

unsigned int count_arguments(const char *);

#endif
