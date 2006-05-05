#ifndef __BYTECODE_CONVERTER_FIXTURE_H
#define __BYTECODE_CONVERTER_FIXTURE_H

struct compilation_unit;
struct methodblock;

static inline struct statement *stmt_entry(struct list_head *head)
{
	return list_entry(head, struct statement, stmt_list_node);
}

struct compilation_unit *alloc_simple_compilation_unit(struct methodblock *);
void assert_value_expr(enum jvm_type, long long, struct tree_node *);
void assert_fvalue_expr(enum jvm_type, double, struct tree_node *);
void assert_local_expr(enum jvm_type, unsigned long, struct tree_node *);
void assert_temporary_expr(unsigned long, struct tree_node *);
void assert_array_deref_expr(enum jvm_type, struct expression *,
			     struct expression *, struct tree_node *);
void __assert_binop_expr(enum jvm_type, enum binary_operator,
			 struct tree_node *);
void assert_binop_expr(enum jvm_type, enum binary_operator,
		       struct expression *, struct expression *,
		       struct tree_node *);
void assert_conv_expr(enum jvm_type, struct expression *, struct tree_node *);
void assert_field_expr(enum jvm_type, struct fieldblock *, struct tree_node *);
void assert_invoke_expr(enum jvm_type, struct methodblock *,
			struct tree_node *);

void assert_store_stmt(struct statement *);
void assert_return_stmt(struct expression *, struct statement *);
void assert_void_return_stmt(struct statement *);
void assert_null_check_stmt(struct expression *, struct statement *);
void assert_arraycheck_stmt(enum jvm_type, struct expression *,
			    struct expression *, struct statement *);

#endif
