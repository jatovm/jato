#ifndef __JIT_TREE_NODE_H
#define __JIT_TREE_NODE_H

/*  Expression type, binary operator, and unary operator are encoded to
    single field because BURG grammar needs all of them to distinguish
    between tree node types.   */
#define EXPR_TYPE_MASK	0x000000FFUL
#define EXPR_TYPE_SHIFT	0UL
#define OP_MASK		0x00FFFF00UL
#define OP_SHIFT	8UL
#define STMT_TYPE_MASK	0xFF000000UL
#define STMT_TYPE_SHIFT	24UL

struct tree_node {
	struct tree_node *kids[2];
	unsigned long op;
	unsigned long bytecode_offset;
};

static inline int node_is_stmt(struct tree_node *node)
{
	return node->op & STMT_TYPE_MASK;
}

static inline int node_is_op(struct tree_node *node)
{
	return node->op & OP_MASK;
}

static inline int tree_op(struct tree_node *node)
{
	if (node_is_stmt(node))
		return (node->op & STMT_TYPE_MASK) >> STMT_TYPE_SHIFT;
	else if (node_is_op(node))
		return (node->op & OP_MASK) >> OP_SHIFT;

	return (node->op & EXPR_TYPE_MASK) >> EXPR_TYPE_SHIFT;
}

int node_nr_kids(struct tree_node *node);

#endif
