/*
 * Copyright (C) 2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 *
 * The file contains functions for printing out intermediate representation
 * tree in human-readable form.
 */

#include <jit/tree-printer.h>
#include <jit/statement.h>
#include <jit/tree-node.h>
#include <vm/string.h>

#include <stdio.h>

static int indent(int lvl, struct string *str)
{
	int err = 0;

	for (int i = 0; i < lvl; i++) {
		err = str_append(str, "  ");
		if (err)
			return err;
	}
	return err;
}

static int append_formatted(int lvl, struct string *str, const char *fmt, ...)
{
	int err;
	va_list args;

	va_start(args, fmt);

	err = indent(lvl, str);
	if (err)
		goto out;

	err = str_vappend(str, fmt, args);

      out:
	va_end(args);
	return err;
}

static int start_attr(int lvl, struct string *str, const char *attr_name)
{
	return append_formatted(lvl, str, "%s:", attr_name);
}

static int append_simple_attr(int lvl, struct string *str,
			      const char *type_name, const char *fmt, ...)
{
	int err = 0;
	va_list args;

	va_start(args, fmt);

	err = start_attr(lvl, str, type_name);
	if (err)
		goto out;

	err = str_append(str, " [");
	if (err)
		goto out;

	err = str_vappend(str, fmt, args);
	if (err)
		goto out;

	err = str_append(str, "]\n");
      out:
	va_end(args);
	return err;
}

static int simple_expr(struct expression *expr)
{
	enum expression_type type = expr_type(expr);

	return type == EXPR_VALUE || type == EXPR_FVALUE || type == EXPR_LOCAL
	    || type == EXPR_TEMPORARY || type == EXPR_FIELD
	    || type == EXPR_NO_ARGS;
}

static int __tree_print(int, struct tree_node *, struct string *);

static int append_tree_attr(int lvl, struct string *str, const char *attr_name,
			    struct tree_node *node)
{
	int err, multiline;

	err = start_attr(lvl, str, attr_name);
	if (err)
		goto out;

	multiline = node && (node_is_stmt(node) || !simple_expr(to_expr(node)));

	if (multiline) {
		err = str_append(str, "\n");
		if (err)
			goto out;

		lvl++;
	} else
		str_append(str, " ");

	err = __tree_print(lvl, node, str);
	if (err)
		goto out;

	if (!multiline)
		err = str_append(str, "\n");

      out:
	return err;
}

static int print_nop_stmt(int lvl, struct string *str, struct statement *stmt)
{
	return append_formatted(lvl, str, "NOP\n");
}

static int print_store_stmt(int lvl, struct string *str, struct statement *stmt)
{
	int err;

	err = append_formatted(lvl, str, "STORE:\n");
	if (err)
		goto out;

	err = append_tree_attr(lvl + 1, str, "store_dest", stmt->store_dest);
	if (err)
		goto out;

	err = append_tree_attr(lvl + 1, str, "store_src", stmt->store_src);

      out:
	return err;
}

static int print_if_stmt(int lvl, struct string *str, struct statement *stmt)
{
	int err;

	err = append_formatted(lvl, str, "IF:\n");
	if (err)
		goto out;

	err = append_tree_attr(lvl + 1, str, "if_conditional",
			       stmt->if_conditional);
	if (err)
		goto out;

	err = append_simple_attr(lvl + 1, str, "if_true", "label %p",
				 stmt->if_true);

      out:
	return err;
}

static int print_label_stmt(int lvl, struct string *str, struct statement *stmt)
{
	return append_formatted(lvl, str, "LABEL: [%p]\n", stmt);
}

static int print_goto_stmt(int lvl, struct string *str, struct statement *stmt)
{
	int err;

	err = append_formatted(lvl, str, "GOTO:\n");
	if (err)
		goto out;

	err = append_simple_attr(lvl + 1, str, "goto_target", "label %p",
			       stmt->goto_target);

      out:
	return err;
}

static int print_return_stmt(int lvl, struct string *str,
			     struct statement *stmt)
{
	int err;

	err = append_formatted(lvl, str, "RETURN:\n");
	if (err)
		goto out;

	err = append_tree_attr(lvl + 1, str, "return_value",
			       stmt->return_value);

      out:
	return err;
}

static int print_void_return_stmt(int lvl, struct string *str,
				  struct statement *stmt)
{
	return append_formatted(lvl, str, "VOID_RETURN\n");
}

static int print_expr_stmt(int lvl, struct string *str, const char *type_name,
			   struct statement *stmt)
{
	int err;

	err = append_formatted(lvl, str, "%s:\n", type_name);
	if (err)
		goto out;

	err = append_tree_attr(lvl + 1, str, "expression", stmt->expression);

      out:
	return err;
}

static int print_expression_stmt(int lvl, struct string *str,
				 struct statement *stmt)
{
	return print_expr_stmt(lvl, str, "EXPRESSION", stmt);
}

static int print_null_check_stmt(int lvl, struct string *str,
				 struct statement *stmt)
{
	return print_expr_stmt(lvl, str, "NULL_CHECK", stmt);
}

static int print_array_check_stmt(int lvl, struct string *str,
				  struct statement *stmt)
{
	return print_expr_stmt(lvl, str, "ARRAY_CHECK", stmt);
}

typedef int (*print_stmt_fn) (int, struct string * str, struct statement *);

static print_stmt_fn stmt_printers[] = {
	[STMT_NOP] = print_nop_stmt,
	[STMT_STORE] = print_store_stmt,
	[STMT_IF] = print_if_stmt,
	[STMT_LABEL] = print_label_stmt,
	[STMT_GOTO] = print_goto_stmt,
	[STMT_RETURN] = print_return_stmt,
	[STMT_VOID_RETURN] = print_void_return_stmt,
	[STMT_EXPRESSION] = print_expression_stmt,
	[STMT_NULL_CHECK] = print_null_check_stmt,
	[STMT_ARRAY_CHECK] = print_array_check_stmt,
};

static int print_stmt(int lvl, struct tree_node *root, struct string *str)
{
	struct statement *stmt = to_stmt(root);
	enum statement_type type = stmt_type(stmt);
	print_stmt_fn print = stmt_printers[type];

	return print(lvl, str, stmt);
}

static const char *type_names[] = {
	[J_VOID] = "void",
	[J_REFERENCE] = "reference",
	[J_BYTE] = "byte",
	[J_SHORT] = "short",
	[J_INT] = "int",
	[J_LONG] = "long",
	[J_CHAR] = "char",
	[J_FLOAT] = "float",
	[J_DOUBLE] = "double",
	[J_BOOLEAN] = "boolean",
	[J_RETURN_ADDRESS] = "return address",
};

static int print_value_expr(int lvl, struct string *str,
			    struct expression *expr)
{
	return str_append(str, "[value %s %llu]", type_names[expr->jvm_type],
			  expr->value);
}

static int print_fvalue_expr(int lvl, struct string *str,
			     struct expression *expr)
{
	return str_append(str, "[fvalue %s %f]", type_names[expr->jvm_type],
			  expr->fvalue);
}

static int print_local_expr(int lvl, struct string *str,
			    struct expression *expr)
{
	return str_append(str, "[local %s %lu]", type_names[expr->jvm_type],
			  expr->local_index);
}

static int print_temporary_expr(int lvl, struct string *str,
				struct expression *expr)
{
	return str_append(str, "[temporary %s %lu]", type_names[expr->jvm_type],
			  expr->local_index);
}

static int print_array_deref_expr(int lvl, struct string *str,
				  struct expression *expr)
{
	int err;

	err = append_formatted(lvl, str, "ARRAY_DEREF:\n");
	if (err)
		goto out;

	err = append_simple_attr(lvl + 1, str, "jvm_type",
				 type_names[expr->jvm_type]);
	if (err)
		goto out;

	err = append_tree_attr(lvl + 1, str, "arrayref", expr->arrayref);
	if (err)
		goto out;

	err = append_tree_attr(lvl + 1, str, "array_index", expr->array_index);

      out:
	return err;
}

static const char *op_names[] = {
	[OP_ADD] = "add",
	[OP_SUB] = "sub",
	[OP_MUL] = "mul",
	[OP_DIV] = "div",
	[OP_REM] = "rem",
	[OP_SHL] = "shl",
	[OP_SHR] = "shr",
	[OP_AND] = "and",
	[OP_OR] = "or",
	[OP_XOR] = "xor",
	[OP_CMP] = "cmp",
	[OP_CMPL] = "cmpl",
	[OP_CMPG] = "cmpg",
	[OP_EQ] = "eq",
	[OP_NE] = "ne",
	[OP_LT] = "lt",
	[OP_GE] = "ge",
	[OP_GT] = "gt",
	[OP_LE] = "le",
	[OP_NEG] = "neg",
};

static int print_binop_expr(int lvl, struct string *str,
			    struct expression *expr)
{
	int err;
	enum binary_operator operator = expr_bin_op(expr);

	err = append_formatted(lvl, str, "BINOP:\n");
	if (err)
		goto out;

	err = append_simple_attr(lvl + 1, str, "jvm_type",
				 type_names[expr->jvm_type]);
	if (err)
		goto out;

	err = append_simple_attr(lvl + 1, str, "binary_operator",
				 op_names[operator]);
	if (err)
		goto out;

	err = append_tree_attr(lvl + 1, str, "binary_left", expr->binary_left);
	if (err)
		goto out;

	err = append_tree_attr(lvl + 1, str, "binary_right", expr->binary_right);

      out:
	return err;
}

static int print_unary_op_expr(int lvl, struct string *str,
			       struct expression *expr)
{
	int err;
	enum unary_operator operator = expr_unary_op(expr);

	err = append_formatted(lvl, str, "UNARY_OP:\n");
	if (err)
		goto out;

	err = append_simple_attr(lvl + 1, str, "jvm_type",
				 type_names[expr->jvm_type]);
	if (err)
		goto out;

	err = append_simple_attr(lvl + 1, str, "unary_operator",
				 op_names[operator]);
	if (err)
		goto out;

	err = append_tree_attr(lvl + 1, str, "unary_expression",
			       expr->unary_expression);

      out:
	return err;
}

static int print_conversion_expr(int lvl, struct string *str,
				 struct expression *expr)
{
	int err;

	err = append_formatted(lvl, str, "CONVERSION:\n");
	if (err)
		goto out;

	err = append_simple_attr(lvl + 1, str, "jvm_type",
				 type_names[expr->jvm_type]);
	if (err)
		goto out;

	err = append_tree_attr(lvl + 1, str, "from_expression",
			       expr->unary_expression);

      out:
	return err;
}

static int print_field_expr(int lvl, struct string *str,
			    struct expression *expr)
{
	return str_append(str, "[field %s %p]", type_names[expr->jvm_type],
			  expr->field);
}

static int print_invoke_expr(int lvl, struct string *str,
			     struct expression *expr)
{
	int err;

	err = append_formatted(lvl, str, "INVOKE:\n");
	if (err)
		goto out;

	err = append_simple_attr(lvl + 1, str, "target_method", "%p",
				 expr->target_method);
	if (err)
		goto out;

	err = append_tree_attr(lvl + 1, str, "args_list", expr->args_list);

      out:
	return err;
}

static int print_args_list_expr(int lvl, struct string *str,
				struct expression *expr)
{
	int err;

	err = append_formatted(lvl, str, "ARGS_LIST:\n");
	if (err)
		goto out;

	err = append_tree_attr(lvl + 1, str, "args_left", expr->args_left);
	if (err)
		goto out;

	err = append_tree_attr(lvl + 1, str, "args_right", expr->args_right);

      out:
	return err;
}

static int print_arg_expr(int lvl, struct string *str, struct expression *expr)
{
	int err;

	err = append_formatted(lvl, str, "ARG:\n");
	if (err)
		goto out;

	err = append_tree_attr(lvl + 1, str, "arg_expression",
			       expr->arg_expression);

      out:
	return err;
}

static int print_no_args_expr(int lvl, struct string *str,
			      struct expression *expr)
{
	return str_append(str, "[no args]");
}

typedef int (*print_expr_fn) (int, struct string * str, struct expression *);

static print_expr_fn expr_printers[] = {
	[EXPR_VALUE] = print_value_expr,
	[EXPR_FVALUE] = print_fvalue_expr,
	[EXPR_LOCAL] = print_local_expr,
	[EXPR_TEMPORARY] = print_temporary_expr,
	[EXPR_ARRAY_DEREF] = print_array_deref_expr,
	[EXPR_BINOP] = print_binop_expr,
	[EXPR_UNARY_OP] = print_unary_op_expr,
	[EXPR_CONVERSION] = print_conversion_expr,
	[EXPR_FIELD] = print_field_expr,
	[EXPR_INVOKE] = print_invoke_expr,
	[EXPR_ARGS_LIST] = print_args_list_expr,
	[EXPR_ARG] = print_arg_expr,
	[EXPR_NO_ARGS] = print_no_args_expr,
};

static int print_expr(int lvl, struct tree_node *root, struct string *str)
{
	struct expression *expr = to_expr(root);
	enum expression_type type = expr_type(expr);
	print_expr_fn print = expr_printers[type];

	return print(lvl, str, expr);
}

static int __tree_print(int lvl, struct tree_node *root, struct string *str)
{
	if (!root)
		return str_append(str, "<null>");

	if (node_is_stmt(root))
		return print_stmt(lvl, root, str);

	return print_expr(lvl, root, str);
}

int tree_print(struct tree_node *root, struct string *str)
{
	return __tree_print(0, root, str);
}
