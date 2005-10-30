#ifndef __OPERAND_H
#define __OPERAND_H

#include <constant.h>
#include <local-variable.h>

enum operand_type {
	OPERAND_CONSTANT,
	OPERAND_LOCAL_VARIABLE,
	OPERAND_TEMPORARY,
};

struct operand {
	enum operand_type o_type;
	union {
		struct constant o_const;	/* OPERAND_CONSTANT */
		struct local_variable o_local;	/* OPERAND_LOCAL_VARIABLE */
		unsigned long o_temporary;	/* OPERAND_TEMPORARY */
	};
};

#endif
