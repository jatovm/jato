#ifndef __BYTECODE_CONVERTER_FIXTURE_H
#define __BYTECODE_CONVERTER_FIXTURE_H

#include <jit/statement.h>
#include <vm/list.h>

struct compilation_unit;
struct basic_block;
struct methodblock;

static inline struct statement *stmt_entry(struct list_head *head)
{
	return list_entry(head, struct statement, stmt_list_node);
}

struct compilation_unit *alloc_simple_compilation_unit(struct methodblock *);
struct basic_block *__alloc_simple_bb(struct methodblock *);
struct basic_block *alloc_simple_bb(unsigned char *, unsigned long);
void __free_simple_bb(struct basic_block *);
void free_simple_bb(struct basic_block *);

void assert_value_expr(enum vm_type, long long, struct tree_node *);
void assert_nullcheck_value_expr(enum vm_type, long long, struct tree_node *);
void assert_fvalue_expr(enum vm_type, double, struct tree_node *);
void assert_local_expr(enum vm_type, unsigned long, struct tree_node *);
void assert_temporary_expr(struct tree_node *);
void assert_array_deref_expr(enum vm_type, struct expression *,
			     struct expression *, struct tree_node *);
void __assert_binop_expr(enum vm_type, enum binary_operator,
			 struct tree_node *);
void assert_binop_expr(enum vm_type, enum binary_operator,
		       struct expression *, struct expression *,
		       struct tree_node *);
void assert_conv_expr(enum vm_type, struct expression *, struct tree_node *);
void assert_class_field_expr(enum vm_type, struct fieldblock *, struct tree_node *);
void assert_instance_field_expr(enum vm_type, struct fieldblock *, struct expression *, struct tree_node *);
void assert_invoke_expr(enum vm_type, struct methodblock *,
			struct tree_node *);

void assert_store_stmt(struct statement *);
void assert_return_stmt(struct expression *, struct statement *);
void assert_void_return_stmt(struct statement *);
void assert_null_check_stmt(struct expression *, struct statement *);
void assert_arraycheck_stmt(enum vm_type, struct expression *,
			    struct expression *, struct statement *);
void assert_monitorenter_stmt(struct expression *, struct statement *);
void assert_monitorexit_stmt(struct expression *, struct statement *);
void assert_checkcast_stmt(struct expression *, struct statement *);

void convert_ir_const(struct compilation_unit *, ConstantPoolEntry *, size_t, u1 *);

struct statement *first_stmt(struct compilation_unit *cu);

#endif
