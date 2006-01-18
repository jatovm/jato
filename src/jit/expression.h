#ifndef __EXPRESSION_H
#define __EXPRESSION_H

#include <constant.h>
#include <local-variable.h>

enum expression_type {
	CONSTANT,
	LOCAL_VAR,
	TEMPORARY,
	ARRAYREF,
};

struct expression {
	enum expression_type type;
	union {
		/* CONSTANT */
		struct constant constant;

		/* LOCAL_VAR */
		struct local_variable local_var;

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
				     enum constant_type type,
				     unsigned long value)
{
	expression->type = CONSTANT;
	expression->constant.type = type;
	expression->constant.value = value;
}

static inline void expression_set_fconst(struct expression *expression,
				      enum constant_type type, double fvalue)
{
	expression->type = CONSTANT;
	expression->constant.type = type;
	expression->constant.fvalue = fvalue;
}

static inline void expression_set_local_var(struct expression *expression,
					 enum jvm_type type,
					 unsigned long index)
{
	expression->type = LOCAL_VAR;
	expression->local_var.type = type;
	expression->local_var.index = index;
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
