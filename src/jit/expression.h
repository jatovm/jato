#ifndef __EXPRESSION_H
#define __EXPRESSION_H

#include <jvm_types.h>

enum expression_type {
	EXPR_VALUE,
	EXPR_FVALUE,
	EXPR_LOCAL,
	EXPR_TEMPORARY,
	EXPR_ARRAY_DEREF,
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

		/* EXPR_TEMPORARY */
		unsigned long temporary;

		/* EXPR_ARRAY_DEREF */
		struct {
			struct expression *arrayref;
			struct expression *array_index;
		};
	};
};

struct expression *alloc_expression(enum expression_type, enum jvm_type);
struct expression *value_expr(enum jvm_type, unsigned long long);
struct expression *fvalue_expr(enum jvm_type, double);
struct expression *local_expr(enum jvm_type, unsigned long);
struct expression *temporary_expr(enum jvm_type, unsigned long);
struct expression *array_deref_expr(enum jvm_type, struct expression *, struct expression *);
void free_expression(struct expression *);

#endif
