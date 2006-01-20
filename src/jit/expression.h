#ifndef __EXPRESSION_H
#define __EXPRESSION_H

#include <jvm_types.h>

enum expression_type {
	EXPR_VALUE,
	EXPR_FVALUE,
	EXPR_LOCAL,
	TEMPORARY,
	ARRAYREF,
};

struct expression {
	enum expression_type type;
	enum jvm_type jvm_type;
	union {
		/* EXPR_VALUE */
		unsigned long long value;

		/* EXPR_FVALUE */
		double fvalue;

		/* EXPR_LOCAL */
		unsigned long local_index;

		/* TEMPORARY */
		unsigned long temporary;

		/* ARRAYREF */
		struct {
			unsigned long arrayref;
			unsigned long array_index;
		};
	};
};

static inline void expression_set_const(struct expression *expression,
				     enum jvm_type type,
				     unsigned long value)
{
	expression->type = EXPR_VALUE;
	expression->jvm_type = type;
	expression->value = value;
}

static inline void expression_set_fconst(struct expression *expression,
				      enum jvm_type type, double fvalue)
{
	expression->type = EXPR_FVALUE;
	expression->jvm_type = type;
	expression->fvalue = fvalue;
}

static inline void expression_set_local_var(struct expression *expression,
					 enum jvm_type type,
					 unsigned long index)
{
	expression->type = EXPR_LOCAL;
	expression->jvm_type = type;
	expression->local_index = index;
}

static inline void expression_set_temporary(struct expression *expression,
					 unsigned long temporary)
{
	expression->type = TEMPORARY;
	expression->temporary = temporary;
}

static inline void expression_set_arrayref(struct expression *expression,
					unsigned long arrayref,
					unsigned long index)
{
	expression->type = ARRAYREF;
	expression->arrayref = arrayref;
	expression->array_index = index;
}

#endif
