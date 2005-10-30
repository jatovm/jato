#ifndef __OPERAND_H
#define __OPERAND_H

#include <constant.h>
#include <local-variable.h>

enum operand_type {
	OPERAND_CONSTANT,
	OPERAND_LOCAL_VARIABLE,
	OPERAND_TEMPORARY,
	OPERAND_ARRAYREF,
};

struct operand {
	enum operand_type o_type;
	union {
		/* OPERAND_CONSTANT */
		struct constant o_const;

		/* OPERAND_LOCAL_VARIABLE */
		struct local_variable o_local;

		/* OPERAND_TEMPORARY */
		unsigned long o_temporary;

		/* OPERAND_ARRAYREF */
		struct {
			unsigned long o_arrayref;
			unsigned long o_index;
		};
	};
};

#endif
