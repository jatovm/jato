/*
 * Copyright (C) 2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 *
 * The file contains functions for printing out intermediate representation
 * tree in human-readable form.
 */

#include "jit/expression.h"
#include "jit/tree-printer.h"
#include "jit/statement.h"
#include "jit/tree-node.h"
#include "jit/vars.h"

#include "vm/class.h"
#include "vm/method.h"
#include "lib/string.h"

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
	    || type == EXPR_TEMPORARY || type == EXPR_CLASS_FIELD
	    || type == EXPR_NO_ARGS || type == EXPR_EXCEPTION_REF
	    || type == EXPR_MIMIC_STACK_SLOT || type == EXPR_FLOAT_LOCAL
	    || type == EXPR_FLOAT_TEMPORARY || type == EXPR_FLOAT_CLASS_FIELD
	    || type == EXPR_FLOAT_INSTANCE_FIELD;
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

	err = append_simple_attr(lvl + 1, str, "if_true", "bb %p",
				 stmt->if_true);

out:
	return err;
}

static int print_goto_stmt(int lvl, struct string *str, struct statement *stmt)
{
	int err;

	err = append_formatted(lvl, str, "GOTO:\n");
	if (err)
		goto out;

	err = append_simple_attr(lvl + 1, str, "goto_target", "bb %p",
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

static int print_array_check_stmt(int lvl, struct string *str,
				  struct statement *stmt)
{
	return print_expr_stmt(lvl, str, "ARRAY_CHECK", stmt);
}

static int print_monitorenter_stmt(int lvl, struct string *str,
				   struct statement *stmt)
{
	return print_expr_stmt(lvl, str, "MONITOR_ENTER", stmt);
}

static int print_monitorexit_stmt(int lvl, struct string *str,
				  struct statement *stmt)
{
	return print_expr_stmt(lvl, str, "MONITOR_EXIT", stmt);
}

static int print_checkcast_stmt(int lvl, struct string *str,
				struct statement *stmt)
{
	int err;

	err = append_formatted(lvl, str, "CHECKCAST:\n");
	if (err)
		goto out;

	err =
	    append_simple_attr(lvl + 1, str, "checkcast_type", "%p '%s'",
			       stmt->checkcast_class,
			       stmt->checkcast_class->name);
	if (err)
		goto out;

	err =
	    append_tree_attr(lvl + 1, str, "checkcast_ref",
			     stmt->checkcast_ref);

out:
	return err;
}

static int print_athrow_stmt(int lvl, struct string *str,
			     struct statement *stmt)
{
	int err;

	err = append_formatted(lvl, str, "ATHROW:\n");
	if (err)
		goto out;

	err = append_tree_attr(lvl + 1, str, "expression", stmt->expression);

out:
	return err;
}

static int print_array_store_check_stmt(int lvl, struct string *str,
					struct statement *stmt)
{
	int err;

	err = append_formatted(lvl, str, "ARRAY_STORE_CHECK:\n");
	if (err)
		goto out;

	err = append_tree_attr(lvl + 1, str, "src", stmt->store_check_src);
	if (err)
		goto out;

	err = append_tree_attr(lvl + 1, str, "array", stmt->store_check_array);

out:
	return err;
}

static int print_tableswitch_stmt(int lvl, struct string *str,
				  struct statement *stmt)
{
	int err;

	err = append_formatted(lvl, str, "TABLESWITCH:\n");
	if (err)
		goto out;

	err = append_tree_attr(lvl + 1, str, "index", stmt->expression);

	unsigned int count = stmt->table->high - stmt->table->low + 1;
	for (unsigned int i = 0; i < count; i++) {
		char case_name[16];

		sprintf(case_name, "case %d", stmt->table->low + i);
		err = append_simple_attr(lvl + 1, str, case_name, "bb %p",
					 stmt->table->bb_lookup_table[i]);
	}

out:
	return err;
}

static int print_lookupswitch_jump_stmt(int lvl, struct string *str,
					struct statement *stmt)
{
	int err;

	err = append_formatted(lvl, str, "LOOKUPSWITCH_JUMP:\n");
	if (err)
		goto out;

	err = append_tree_attr(lvl + 1, str, "target",
			       stmt->lookupswitch_target);

 out:
	return err;
}

typedef int (*print_stmt_fn) (int, struct string * str, struct statement *);

static print_stmt_fn stmt_printers[] = {
	[STMT_STORE] = print_store_stmt,
	[STMT_IF] = print_if_stmt,
	[STMT_GOTO] = print_goto_stmt,
	[STMT_RETURN] = print_return_stmt,
	[STMT_VOID_RETURN] = print_void_return_stmt,
	[STMT_EXPRESSION] = print_expression_stmt,
	[STMT_ARRAY_CHECK] = print_array_check_stmt,
	[STMT_MONITOR_ENTER] = print_monitorenter_stmt,
	[STMT_MONITOR_EXIT] = print_monitorexit_stmt,
	[STMT_CHECKCAST] = print_checkcast_stmt,
	[STMT_ATHROW] = print_athrow_stmt,
	[STMT_ARRAY_STORE_CHECK] = print_array_store_check_stmt,
	[STMT_TABLESWITCH] = print_tableswitch_stmt,
	[STMT_LOOKUPSWITCH_JUMP] = print_lookupswitch_jump_stmt,
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
	return str_append(str, "[value %s 0x%llx]", type_names[expr->vm_type],
			  expr->value);
}

static int print_float_local_expr(int lvl, struct string *str,
			    struct expression *expr)
{
	return str_append(str, "[float-local %s %lu]",
			  type_names[expr->vm_type],
			  expr->local_index);
}

static int print_fvalue_expr(int lvl, struct string *str,
			     struct expression *expr)
{
	return str_append(str, "[fvalue %s %f]", type_names[expr->vm_type],
			  expr->fvalue);
}

static int print_local_expr(int lvl, struct string *str,
			    struct expression *expr)
{
	return str_append(str, "[local %s %lu]", type_names[expr->vm_type],
			  expr->local_index);
}

static int print_temporary_expr(int lvl, struct string *str,
				struct expression *expr)
{
	int err;

	err = str_append(str, "[temporary %s ", type_names[expr->vm_type]);
	if (err)
		goto out;

	if (expr->tmp_high) {
		err = str_append(str, "0x%lx (high), ", expr->tmp_high);
		if (err)
			goto out;
	}

	err = str_append(str, "0x%lx (low)]", expr->tmp_low);
	if (err)
		goto out;

out:
	return err;
}

static int print_float_temporary_expr(int lvl, struct string *str,
				      struct expression *expr)
{
	int err;

	err = str_append(str, "[float-temporary %s ",
			 type_names[expr->vm_type]);
	if (err)
		goto out;

	if (expr->tmp_high) {
		err = str_append(str, "0x%lx (high), ", expr->tmp_high);
		if (err)
			goto out;
	}

	err = str_append(str, "0x%lx (low)]", expr->tmp_low);
	if (err)
		goto out;

out:
	return err;
}

static int print_mimic_stack_slot_expr(int lvl, struct string *str,
				       struct expression *expr)
{
	return str_append(str, "[mimic stack slot %d at %s]", expr->slot_ndx,
			  expr->entry ? "entry" : "exit");
}

static int print_array_deref_expr(int lvl, struct string *str,
				  struct expression *expr)
{
	int err;

	err = append_formatted(lvl, str, "ARRAY_DEREF:\n");
	if (err)
		goto out;

	err = append_simple_attr(lvl + 1, str, "vm_type",
				 type_names[expr->vm_type]);
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
	[OP_MUL_64] = "mul64",
	[OP_DIV] = "div",
	[OP_DIV_64] = "div64",
	[OP_REM] = "rem",
	[OP_REM_64] = "rem64",
	[OP_FADD] = "fadd",
	[OP_FSUB] = "fsub",
	[OP_FMUL] = "fmul",
	[OP_FNEG] = "fneg",
	[OP_FDIV] = "fdiv",
	[OP_FREM] = "frem",
	[OP_DADD] = "dadd",
	[OP_DSUB] = "dsub",
	[OP_DMUL] = "mul",
	[OP_DNEG] = "dneg",
	[OP_DDIV] = "ddiv",
	[OP_DREM] = "drem",
	[OP_SHL] = "shl",
	[OP_SHL_64] = "shl64",
	[OP_SHR] = "shr",
	[OP_SHR_64] = "shr64",
	[OP_USHR] = "ushr",
	[OP_USHR_64] = "ushr64",
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

	err = append_simple_attr(lvl + 1, str, "vm_type",
				 type_names[expr->vm_type]);
	if (err)
		goto out;

	err = append_simple_attr(lvl + 1, str, "binary_operator",
				 op_names[operator]);
	if (err)
		goto out;

	err = append_tree_attr(lvl + 1, str, "binary_left", expr->binary_left);
	if (err)
		goto out;

	err =
	    append_tree_attr(lvl + 1, str, "binary_right", expr->binary_right);

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

	err = append_simple_attr(lvl + 1, str, "vm_type",
				 type_names[expr->vm_type]);
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

	err = append_simple_attr(lvl + 1, str, "vm_type",
				 type_names[expr->vm_type]);
	if (err)
		goto out;

	err = append_tree_attr(lvl + 1, str, "from_expression",
			       expr->unary_expression);

out:
	return err;
}

static int print_truncation_expr(int lvl, struct string *str,
				 struct expression *expr)
{
	int err;

	err = append_formatted(lvl, str, "TRUNCATION:\n");
	if (err)
		goto out;

	err = append_simple_attr(lvl + 1, str, "to_type",
				 type_names[expr->to_type]);
	if (err)
		goto out;

	err = append_tree_attr(lvl + 1, str, "from_expression",
			       expr->unary_expression);

out:
	return err;
}

static int print_class_field_expr(int lvl, struct string *str,
				  struct expression *expr)
{
	return str_append(str, "[class_field %s %p '%s.%s']",
			  type_names[expr->vm_type], expr->class_field,
			  expr->class_field->class->name,
			  expr->class_field->name);
}

static int print_float_class_field_expr(int lvl, struct string *str,
					struct expression *expr)
{
	return str_append(str, "[float_class_field %s %p '%s.%s']",
			  type_names[expr->vm_type], expr->class_field,
			  expr->class_field->class->name,
			  expr->class_field->name);
}

static int print_instance_field_expr(int lvl, struct string *str,
				     struct expression *expr)
{
	int err;

	err = append_formatted(lvl, str, "INSTANCE_FIELD:\n");
	if (err)
		goto out;

	err =
	    append_simple_attr(lvl + 1, str, "vm_type",
			       type_names[expr->vm_type]);
	if (err)
		goto out;

	err =
	    append_simple_attr(lvl + 1, str, "instance_field", "%p '%s.%s'",
			       expr->instance_field,
			       expr->instance_field->class->name,
			       expr->instance_field->name);
	if (err)
		goto out;

	err =
	    append_tree_attr(lvl + 1, str, "objectref_expression",
			     expr->objectref_expression);

out:
	return err;
}

static int print_float_instance_field_expr(int lvl, struct string *str,
					   struct expression *expr)
{
	int err;

	err = append_formatted(lvl, str, "FLOAT_INSTANCE_FIELD:\n");
	if (err)
		goto out;

	err =
	    append_simple_attr(lvl + 1, str, "vm_type",
			       type_names[expr->vm_type]);
	if (err)
		goto out;

	err =
	    append_simple_attr(lvl + 1, str, "instance_field", "%p '%s.%s'",
			       expr->instance_field,
			       expr->instance_field->class->name,
			       expr->instance_field->name);
	if (err)
		goto out;

	err =
	    append_tree_attr(lvl + 1, str, "objectref_expression",
			     expr->objectref_expression);

out:
	return err;
}

static int print_invoke_expr(int lvl, struct string *str,
			     struct expression *expr)
{
	struct vm_method *method;
	int err;

	err = append_formatted(lvl, str, "INVOKE:\n");
	if (err)
		goto out;

	method = expr->target_method;

	err = append_simple_attr(lvl + 1, str, "target_method", "%p '%s.%s%s'",
				 method, method->class->name, method->name,
				 method->type);
	if (err)
		goto out;

	err = append_tree_attr(lvl + 1, str, "args_list", expr->args_list);

out:
	return err;
}

static int __print_invoke_expr(int lvl, struct string *str,
			       struct expression *expr, const char *name)
{
	struct vm_method *method;
	int err;

	err = append_formatted(lvl, str, "%s:\n", name);
	if (err)
		goto out;

	method = expr->target_method;

	err =
	    append_simple_attr(lvl + 1, str, "target_method",
			       "%p '%s.%s%s' (%lu)", method,
			       method->class->name, method->name, method->type,
			       expr_method_index(expr));
	if (err)
		goto out;

	err = append_tree_attr(lvl + 1, str, "args_list", expr->args_list);

out:
	return err;
}

static int print_invokeinterface_expr(int lvl, struct string *str,
				    struct expression *expr)
{
	return __print_invoke_expr(lvl, str, expr, "INVOKEINTERFACE");
}

static int print_invokevirtual_expr(int lvl, struct string *str,
				    struct expression *expr)
{
	return __print_invoke_expr(lvl, str, expr, "INVOKEVIRTUAL");
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

static int
print_arg_this_expr(int lvl, struct string *str, struct expression *expr)
{
	int err;

	err = append_formatted(lvl, str, "ARG_THIS:\n");
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

static int print_new_expr(int lvl, struct string *str, struct expression *expr)
{
	int err;

	err = append_formatted(lvl, str, "NEW:\n");
	if (err)
		goto out;

	err = append_simple_attr(lvl + 1, str, "vm_type",
				 type_names[expr->vm_type]);
	if (err)
		goto out;

	err =
	    append_simple_attr(lvl + 1, str, "class", "%p '%s'", expr->class,
			       expr->class->name);

out:
	return err;
}

static int print_newarray_expr(int lvl, struct string *str,
			       struct expression *expr)
{
	int err;

	err = append_formatted(lvl, str, "NEWARRAY:\n");
	if (err)
		goto out;

	err =
	    append_simple_attr(lvl + 1, str, "vm_type",
			       type_names[expr->vm_type]);
	if (err)
		goto out;

	err = append_tree_attr(lvl + 1, str, "array_size", expr->array_size);
	if (err)
		goto out;

	err =
	    append_simple_attr(lvl + 1, str, "array_type", "%lu",
			       expr->array_type);

out:
	return err;
}

static int print_anewarray_expr(int lvl, struct string *str,
				struct expression *expr)
{
	int err;

	err = append_formatted(lvl, str, "ANEWARRAY:\n");
	if (err)
		goto out;

	err =
	    append_simple_attr(lvl + 1, str, "vm_type",
			       type_names[expr->vm_type]);
	if (err)
		goto out;

	err =
	    append_tree_attr(lvl + 1, str, "anewarray_size",
			     expr->anewarray_size);
	if (err)
		goto out;

	err =
	    append_simple_attr(lvl + 1, str, "anewarray_ref_type", "%p '%s'",
			       expr->anewarray_ref_type,
			       expr->anewarray_ref_type->name);

out:
	return err;
}

static int print_multianewarray_expr(int lvl, struct string *str,
				     struct expression *expr)
{
	int err;

	err = append_formatted(lvl, str, "MULTIANEWARRAY:\n");
	if (err)
		goto out;

	err =
	    append_simple_attr(lvl + 1, str, "vm_type",
			       type_names[expr->vm_type]);
	if (err)
		goto out;

	err =
	    append_simple_attr(lvl + 1, str, "multianewarray_ref_type",
			       "%p '%s'", expr->multianewarray_ref_type,
			       expr->multianewarray_ref_type->name);
	if (err)
		goto out;

	err =
	    append_tree_attr(lvl + 1, str, "dimension list",
			     expr->multianewarray_dimensions);

out:
	return err;
}

static int print_arraylength_expr(int lvl, struct string *str,
				  struct expression *expr)
{
	int err;

	err = append_formatted(lvl, str, "ARRAYLENGTH:\n");
	if (err)
		goto out;

	err =
	    append_simple_attr(lvl + 1, str, "vm_type",
			       type_names[expr->vm_type]);
	if (err)
		goto out;

	err =
	    append_tree_attr(lvl + 1, str, "arraylength_ref",
			     expr->arraylength_ref);

out:
	return err;
}

static int print_instanceof_expr(int lvl, struct string *str,
				 struct expression *expr)
{
	int err;

	err = append_formatted(lvl, str, "INSTANCEOF:\n");
	if (err)
		goto out;

	err =
	    append_simple_attr(lvl + 1, str, "vm_type",
			       type_names[expr->vm_type]);
	if (err)
		goto out;

	err =
	    append_simple_attr(lvl + 1, str, "instanceof_class", "%p '%s'",
			       expr->instanceof_class,
			       expr->instanceof_class->name);
	if (err)
		goto out;

	err =
	    append_tree_attr(lvl + 1, str, "instanceof_ref",
			     expr->instanceof_ref);

out:
	return err;
}

static int print_exception_ref_expr(int lvl, struct string *str,
				    struct expression *expr)
{
	return str_append(str, "[exception object reference]\n");
}

static int print_null_check_expr(int lvl, struct string *str,
				 struct expression *expr)
{
	int err;

	err = append_formatted(lvl, str, "NULL_CHECK:\n");
	if (err)
		goto out;

	err = append_tree_attr(lvl + 1, str, "ref", expr->null_check_ref);

out:
	return err;
}

static int print_array_size_check_expr(int lvl, struct string *str,
				       struct expression *expr)
{
	int err;

	err = append_formatted(lvl, str, "ARRAY_SIZE_CHECK:\n");
	if (err)
		goto out;

	err = append_tree_attr(lvl + 1, str, "size", expr->size_expr);

out:
	return err;
}

static int print_multiarray_size_check_expr(int lvl, struct string *str,
					    struct expression *expr)
{
	int err;

	err = append_formatted(lvl, str, "MULTIARRAY_SIZE_CHECK:\n");
	if (err)
		goto out;

	err = append_tree_attr(lvl + 1, str, "dimension list", expr->size_expr);

out:
	return err;
}

static int print_lookupswitch_bsearch_expr(int lvl, struct string *str,
					   struct expression *expr)
{
	int err;

	err = append_formatted(lvl, str, "LOOKUPSWITCH_BSEARCH:\n");
	if (err)
		goto out;

	struct lookupswitch *table = expr->lookupswitch_table;

	for (unsigned int i = 0; i < table->count; i++) {
		char case_name[16];

		sprintf(case_name, "case %d", table->pairs[i].match);
		err = append_simple_attr(lvl + 1, str, case_name, "bb %p",
					 table->pairs[i].bb_target);
	}

out:
	return err;
}

typedef int (*print_expr_fn) (int, struct string * str, struct expression *);

static print_expr_fn expr_printers[] = {
	[EXPR_VALUE] = print_value_expr,
	[EXPR_FLOAT_LOCAL] = print_float_local_expr,
	[EXPR_FLOAT_TEMPORARY] = print_float_temporary_expr,
	[EXPR_FVALUE] = print_fvalue_expr,
	[EXPR_LOCAL] = print_local_expr,
	[EXPR_TEMPORARY] = print_temporary_expr,
	[EXPR_ARRAY_DEREF] = print_array_deref_expr,
	[EXPR_BINOP] = print_binop_expr,
	[EXPR_UNARY_OP] = print_unary_op_expr,
	[EXPR_TRUNCATION] = print_truncation_expr,
	[EXPR_CONVERSION] = print_conversion_expr,
	[EXPR_CONVERSION_FLOAT_TO_DOUBLE] = print_conversion_expr,
	[EXPR_CONVERSION_DOUBLE_TO_FLOAT] = print_conversion_expr,
	[EXPR_CONVERSION_FROM_FLOAT] = print_conversion_expr,
	[EXPR_CONVERSION_TO_FLOAT] = print_conversion_expr,
	[EXPR_CONVERSION_FROM_DOUBLE] = print_conversion_expr,
	[EXPR_CONVERSION_TO_DOUBLE] = print_conversion_expr,
	[EXPR_CLASS_FIELD] = print_class_field_expr,
	[EXPR_FLOAT_CLASS_FIELD] = print_float_class_field_expr,
	[EXPR_INSTANCE_FIELD] = print_instance_field_expr,
	[EXPR_FLOAT_INSTANCE_FIELD] = print_float_instance_field_expr,
	[EXPR_INVOKE] = print_invoke_expr,
	[EXPR_INVOKEINTERFACE] = print_invokeinterface_expr,
	[EXPR_INVOKEVIRTUAL] = print_invokevirtual_expr,
	[EXPR_FINVOKE] = print_invoke_expr,
	[EXPR_FINVOKEVIRTUAL] = print_invokevirtual_expr,
	[EXPR_ARGS_LIST] = print_args_list_expr,
	[EXPR_ARG] = print_arg_expr,
	[EXPR_ARG_THIS] = print_arg_this_expr,
	[EXPR_NO_ARGS] = print_no_args_expr,
	[EXPR_NEW] = print_new_expr,
	[EXPR_NEWARRAY] = print_newarray_expr,
	[EXPR_ANEWARRAY] = print_anewarray_expr,
	[EXPR_MULTIANEWARRAY] = print_multianewarray_expr,
	[EXPR_ARRAYLENGTH] = print_arraylength_expr,
	[EXPR_INSTANCEOF] = print_instanceof_expr,
	[EXPR_EXCEPTION_REF] = print_exception_ref_expr,
	[EXPR_NULL_CHECK] = print_null_check_expr,
	[EXPR_ARRAY_SIZE_CHECK] = print_array_size_check_expr,
	[EXPR_MULTIARRAY_SIZE_CHECK] = print_multiarray_size_check_expr,
	[EXPR_MIMIC_STACK_SLOT] = print_mimic_stack_slot_expr,
	[EXPR_LOOKUPSWITCH_BSEARCH] = print_lookupswitch_bsearch_expr,
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
