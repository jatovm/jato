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

static inline void operand_set_const(struct operand *operand,
				     enum constant_type type,
				     unsigned long value)
{
	operand->type = OPERAND_CONSTANT;
	operand->constant.type = type;
	operand->constant.value = value;
}

static inline void operand_set_fconst(struct operand *operand,
				      enum constant_type type, double fvalue)
{
	operand->type = OPERAND_CONSTANT;
	operand->constant.type = type;
	operand->constant.fvalue = fvalue;
}

static inline void operand_set_local_var(struct operand *operand,
					 enum jvm_type type,
					 unsigned long index)
{
	operand->type = OPERAND_LOCAL_VAR;
	operand->local_var.type = type;
	operand->local_var.index = index;
}

static inline void operand_set_temporary(struct operand *operand,
					 unsigned long temporary)
{
	operand->type = OPERAND_TEMPORARY;
	operand->temporary = temporary;
}

static inline void operand_set_arrayref(struct operand *operand,
					unsigned long arrayref,
					unsigned long index)
{
	operand->type = OPERAND_ARRAYREF;
	operand->arrayref = arrayref;
	operand->array_index = index;
}

#endif
