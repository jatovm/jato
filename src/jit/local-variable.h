#ifndef __LOCAL_VARIABLE_H
#define __LOCAL_VARIABLE_H

enum local_variable_type {
	LOCAL_VAR_BOOLEAN,
	LOCAL_VAR_BYTE,
	LOCAL_VAR_CHAR,
	LOCAL_VAR_SHORT,
	LOCAL_VAR_INT,
	LOCAL_VAR_FLOAT,
	LOCAL_VAR_REFERENCE,
	LOCAL_VAR_RETURN_ADDRESS,
	LOCAL_VAR_LONG,
	LOCAL_VAR_DOUBLE
};

struct local_variable {
	enum local_variable_type type;
	unsigned long index;
};

#endif
