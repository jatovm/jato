#ifndef __JVM_TYPES_H
#define __JVM_TYPES_H

enum jvm_type {
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

extern enum jvm_type str_to_type(char *);

#endif
