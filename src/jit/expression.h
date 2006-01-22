#ifndef __EXPRESSION_H
#define __EXPRESSION_H

#include <jvm_types.h>

enum binary_operator {
	OP_ADD,
	OP_SUB,
	OP_MUL,
	OP_DIV,
	OP_REM,
	OP_SHL,
	OP_SHR,
	OP_AND,
	OP_OR,
	OP_XOR,
};

enum unary_operator {
	OP_NEG,
};

enum expression_type {
	EXPR_VALUE,
	EXPR_FVALUE,
	EXPR_LOCAL,
	EXPR_TEMPORARY,
	EXPR_ARRAY_DEREF,
	EXPR_BINOP,
	EXPR_UNARY_OP,
};

struct expression {
	enum expression_type type;
	unsigned long refcount;
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

		/* EXPR_BINOP */
		struct {
			enum binary_operator binary_operator;
			struct expression *left;
			struct expression *right;
		};

		/* EXPR_UNARY_OP */
		struct {
			enum unary_operator unary_operator;
			struct expression *expression;
		};
	};
};

struct expression *alloc_expression(enum expression_type, enum jvm_type);
void free_expression(struct expression *);

void expr_get(struct expression *);
void expr_put(struct expression *);

struct expression *value_expr(enum jvm_type, unsigned long long);
struct expression *fvalue_expr(enum jvm_type, double);
struct expression *local_expr(enum jvm_type, unsigned long);
struct expression *temporary_expr(enum jvm_type, unsigned long);
struct expression *array_deref_expr(enum jvm_type, struct expression *, struct expression *);
struct expression *binop_expr(enum jvm_type, enum binary_operator, struct expression *, struct expression *);
struct expression *unary_op_expr(enum jvm_type, enum unary_operator, struct expression *);

#endif
