#ifndef __EXPRESSION_H
#define __EXPRESSION_H

#include <constant.h>
#include <local-variable.h>

enum expression_type {
	OPERAND_CONSTANT,
	OPERAND_LOCAL_VAR,
	OPERAND_TEMPORARY,
	OPERAND_ARRAYREF,
};

struct expression {
	enum expression_type type;
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

static inline void expression_set_const(struct expression *expression,
				     enum constant_type type,
				     unsigned long value)
{
	expression->type = OPERAND_CONSTANT;
	expression->constant.type = type;
	expression->constant.value = value;
}

static inline void expression_set_fconst(struct expression *expression,
				      enum constant_type type, double fvalue)
{
	expression->type = OPERAND_CONSTANT;
	expression->constant.type = type;
	expression->constant.fvalue = fvalue;
}

static inline void expression_set_local_var(struct expression *expression,
					 enum jvm_type type,
					 unsigned long index)
{
	expression->type = OPERAND_LOCAL_VAR;
	expression->local_var.type = type;
	expression->local_var.index = index;
}

static inline void expression_set_temporary(struct expression *expression,
					 unsigned long temporary)
{
	expression->type = OPERAND_TEMPORARY;
	expression->temporary = temporary;
}

static inline void expression_set_arrayref(struct expression *expression,
					unsigned long arrayref,
					unsigned long index)
{
	expression->type = OPERAND_ARRAYREF;
	expression->arrayref = arrayref;
	expression->array_index = index;
}

#endif
