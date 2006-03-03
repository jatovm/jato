#ifndef __EXPRESSION_H
#define __EXPRESSION_H

#include <jvm_types.h>
#include <list.h>

enum binary_operator {
	/* Arithmetic */
	OP_ADD,
	OP_SUB,
	OP_MUL,
	OP_DIV,
	OP_REM,

	/* Bitwise */
	OP_SHL,
	OP_SHR,
	OP_AND,
	OP_OR,
	OP_XOR,

	/* Comparison */
	OP_CMP,
	OP_CMPL,
	OP_CMPG,

	/* Conditionals */
	OP_EQ,
	OP_NE,
	OP_LT,
	OP_GE,
	OP_GT,
	OP_LE,
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
	EXPR_CONVERSION,
	EXPR_FIELD,
	EXPR_INVOKE,
};

struct expression {
	enum expression_type type;
	unsigned long refcount;
	enum jvm_type jvm_type;
	struct list_head list_node;
	union {
		/*  EXPR_VALUE represents an integer or reference constant
		    expression (see JLS 15.28.). This expression type can be
		    used as an rvalue only.  */
		unsigned long long value;

		/*  EXPR_FVALUE represents a floating-point constant
		    expression (see JLS 15.28.). This expression type can be
		    used as an rvalue only.  */
		double fvalue;

		/*  EXPR_LOCAL represents a local variable or parameter
		    expression (see JLS 15.14. and 6.5.6.). This expression
		    type can be used as both lvalue and rvalue.  */
		unsigned long local_index;

		/*  EXPR_TEMPORARY represents a compiler-generated temporary
		    expression. This expression type can be used as both
		    lvalue and rvalue.  */
		unsigned long temporary;

		/*  EXPR_ARRAY_DEREF represents an array access expression
		    (see JLS 15.13.). This expression type can be used as
		    both lvalue and rvalue.  */
		struct {
			struct expression *arrayref;
			struct expression *array_index;
		};

		/*  EXPR_BINOP represents an binary operation expression (see
		    JLS 15.17., 15.18., 15.19., 15.20., 15.21., 15.22.). This
		    expression type can be used as an rvalue only.  */
		struct {
			enum binary_operator binary_operator;
			struct expression *binary_left;
			struct expression *binary_right;
		};

		/*  EXPR_UNARY_OP represents an unary operation expression
		    (see JLS 15.15.). This expression type can be used as an
		    rvalue only.  */
		struct {
			enum unary_operator unary_operator;
			struct expression *unary_expression;
		};

		/*  EXPR_CONVERSION represents a type conversion (see JLS
		    5.1.).  This expression type can be used as an rvalue
		    only.  */
		struct {
			struct expression *from_expression;
		};

		/*  EXPR_FIELD represents a field access expression (see JLS
		    15.11.). This expression type can be used as both lvalue
		    and rvalue.  */
		struct {
			struct fieldblock *field;
		};
		
		/*  EXPR_INVOKE represents a method invocation expression (see
		    JLS 15.12.). This expression type can contain side-effects
		    and can be used as an rvalue only.  */
		struct {
			struct methodblock *target_method;
			struct list_head args_list;
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
struct expression *conversion_expr(enum jvm_type, struct expression *);
struct expression *field_expr(enum jvm_type, struct fieldblock *);
struct expression *invoke_expr(enum jvm_type, struct methodblock *);

#endif
