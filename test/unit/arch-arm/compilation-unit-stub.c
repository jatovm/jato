#include "jit/compilation-unit.h"
#include "jit/basic-block.h"
#include "jit/statement.h"
#include "jit/expression.h"
#include "jit/bc-offset-mapping.h"
#include "lib/list.h"
#include <stdio.h>

struct compilation_unit *stub_compilation_unit_alloc(void)
{
	struct compilation_unit *cu = malloc(sizeof *cu);
	if (cu) {
		memset(cu, 0, sizeof *cu);

		INIT_LIST_HEAD(&cu->bb_list);

		cu->lir_insn_map = NULL;

		cu->nr_vregs	= NR_FIXED_REGISTERS;
		cu->arena	= arena_new();

		/*
		 * Initialisation of the constant pool linked list
		 */

		cu->pool_head = NULL;
		cu->nr_entries_in_pool = 0;
	}

	return cu;

}

struct expression *
dup_expr(struct parse_context *ctx, struct expression *expr)
{
	struct expression *dest;
	struct statement *stmt;

	dest = temporary_expr(expr->vm_type, ctx->cu);

	expr_get(dest);
	stmt = alloc_statement(STMT_STORE);
	stmt->store_dest = &dest->node;
	stmt->store_src  = &expr->node;
	convert_statement(ctx, stmt);

	return dest;
}

int vm_class_resolve_interface_method(const struct vm_class *vmc, uint16_t i,
	struct vm_class **r_vmc, char **r_name, char **r_type)
{
	return 0;
}

int vm_class_resolve_method(const struct vm_class *vmc, uint16_t i,
	struct vm_class **r_vmc, char **r_name, char **r_type)
{
	return 0;
}
