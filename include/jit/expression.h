#ifndef __EXPRESSION_H
#define __EXPRESSION_H

#include <vm/system.h>
#include <vm/types.h>

#include <jit/tree-node.h>

enum expression_type {
	EXPR_VALUE,
	EXPR_FVALUE,
	EXPR_LOCAL,
	EXPR_TEMPORARY,
	EXPR_ARRAY_DEREF,
	EXPR_BINOP,
	EXPR_UNARY_OP,
	EXPR_CONVERSION,
	EXPR_CLASS_FIELD,
	EXPR_INVOKE,
	EXPR_INVOKEVIRTUAL,
	EXPR_ARGS_LIST,
	EXPR_ARG,
	EXPR_NO_ARGS,
	EXPR_NEW,
	EXPR_LAST,	/* Not a real type. Keep this last. */
};

enum binary_operator {
	OP_ADD = EXPR_LAST,
	OP_SUB,
	OP_MUL,
	OP_DIV,
	OP_REM,

	OP_SHL,
	OP_SHR,
	OP_USHR,
	OP_AND,
	OP_OR,
	OP_XOR,

	OP_CMP,
	OP_CMPL,
	OP_CMPG,

	OP_EQ,
	OP_NE,
	OP_LT,
	OP_GE,
	OP_GT,
	OP_LE,

	OP_LAST,	/* Not a real operator. Keep this last. */
};

enum unary_operator {
	OP_NEG	= OP_LAST,
};

struct expression {
	unsigned long refcount;
	enum vm_type vm_type;
	union {
		struct tree_node node;

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
			struct tree_node *arrayref;
			struct tree_node *array_index;
		};

		/*  EXPR_BINOP represents an binary operation expression (see
		    JLS 15.17., 15.18., 15.19., 15.20., 15.21., 15.22.). This
		    expression type can be used as an rvalue only.  */
		struct {
			struct tree_node *binary_left;
			struct tree_node *binary_right;
		};

		/*  EXPR_UNARY_OP represents an unary operation expression
		    (see JLS 15.15.). This expression type can be used as an
		    rvalue only.  */
		struct {
			struct tree_node *unary_expression;
		};

		/*  EXPR_CONVERSION represents a type conversion (see JLS
		    5.1.).  This expression type can be used as an rvalue
		    only.  */
		struct {
			struct tree_node *from_expression;
		};

		/*  EXPR_CLASS_FIELD represents a static field access
		    expression (see JLS 15.11.). This expression type can be
		    used as both lvalue and rvalue.  */
		struct {
			struct fieldblock *field;
		};
		
		/*  EXPR_INVOKE represents a method invocation expression (see
		    JLS 15.12.) for which the target method can be determined
		    statically.  This expression type can contain side-effects
		    and can be used as an rvalue only.  See also the
		    EXPR_INVOKEVIRTUAL expression type.  */
		struct {
			struct tree_node *args_list;
			struct methodblock *target_method;
		};

		/*  EXPR_INVOKESPECIAL and EXPR_INVOKEVIRTUAL represent a
		    method invocation (see JLS 15.12.) for which the target
		    method has to be determined from instance class vtable at
		    the call site.
		    The first argument in the argument list is always the
		    object reference for the invocation.  This expression type
		    can contain side-effects and can be used as an rvalue
		    only.  See also the EXPR_INVOKE expression type.  */
		struct {
			struct tree_node *args_list;
			unsigned long method_index;
		};

		/*  EXPR_ARGS_LIST represents list of arguments passed to
		    method. This expression does not evaluate to a value and
		    is used for instruction selection only.  */
		struct {
			struct tree_node *args_left;
			struct tree_node *args_right;
		};

		/*  EXPR_ARG represents an argument passed to method. This
		    expression does not evaluate to a value and is used for
		    instruction selection only.  */
		struct {
			struct tree_node *arg_expression;
		};

		/*  EXPR_NO_ARGS is used for EXPR_INVOKE expression type when
		    there are no arguments to pass.  */
		struct {
			/* Nothing. */
		};
		
		/*  EXPR_NEW represents creation of a new instance that is
		    unitialized .  */
		struct {
			struct object *class;
		};
	};
};

static inline struct expression *to_expr(struct tree_node *node)
{
	return container_of(node, struct expression, node);
}

static inline enum expression_type expr_type(struct expression *expr)
{
	return (expr->node.op & EXPR_TYPE_MASK) >> EXPR_TYPE_SHIFT;
}

static inline enum binary_operator expr_bin_op(struct expression *expr)
{
	return (expr->node.op & OP_MASK) >> OP_SHIFT;
}

static inline enum unary_operator expr_unary_op(struct expression *expr)
{
	return (expr->node.op & OP_MASK) >> OP_SHIFT;
}

struct expression *alloc_expression(enum expression_type, enum vm_type);
void free_expression(struct expression *);

void expr_get(struct expression *);
void expr_put(struct expression *);

struct expression *value_expr(enum vm_type, unsigned long long);
struct expression *fvalue_expr(enum vm_type, double);
struct expression *local_expr(enum vm_type, unsigned long);
struct expression *temporary_expr(enum vm_type, unsigned long);
struct expression *array_deref_expr(enum vm_type, struct expression *, struct expression *);
struct expression *binop_expr(enum vm_type, enum binary_operator, struct expression *, struct expression *);
struct expression *unary_op_expr(enum vm_type, enum unary_operator, struct expression *);
struct expression *conversion_expr(enum vm_type, struct expression *);
struct expression *field_expr(enum vm_type, struct fieldblock *);
struct expression *__invoke_expr(enum vm_type, struct methodblock *);
struct expression *__invokevirtual_expr(enum vm_type, unsigned long);
struct expression *invoke_expr(struct methodblock *);
struct expression *invokevirtual_expr(struct methodblock *);
struct expression *args_list_expr(struct expression *, struct expression *);
struct expression *arg_expr(struct expression *);
struct expression *no_args_expr(void);
struct expression *new_expr(struct object *);

unsigned long nr_args(struct expression *);

static inline int is_invoke_expr(struct expression *expr)
{
	enum expression_type type = expr_type(expr);

	return (type == EXPR_INVOKE)
		|| (type == EXPR_INVOKEVIRTUAL);
}

#endif
