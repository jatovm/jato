#ifndef __OPERAND_H
#define __OPERAND_H

#include <constant.h>
#include <local-variable.h>

enum operand_type {
	OPERAND_CONSTANT,
	OPERAND_LOCAL_VAR,
	OPERAND_TEMPORARY,
	OPERAND_ARRAYREF,
};

struct operand {
	enum operand_type type;
	union {
		/* OPERAND_CONSTANT */
		struct constant constant;

		/* OPERAND_LOCAL_VAR */
		struct local_variable local_var;

		/* OPERAND_TEMPORARY */
		unsigned long temporary;

		/* OPERAND_ARRAYREF */
		struct {
			unsigned long arrayref;
			unsigned long array_index;
		};
	};
};

#endif
