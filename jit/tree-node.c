/*
 * Copyright (c) 2009 Tomasz Grabiec
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */

#include "jit/expression.h"
#include "jit/statement.h"

int node_nr_kids(struct tree_node *node)
{
       if (node_is_stmt(node))
               return stmt_nr_kids(to_stmt(node));

       return expr_nr_kids(to_expr(node));
}
