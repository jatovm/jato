#ifndef _BC_OFFSET_MAPPING
#define _BC_OFFSET_MAPPING

#include <jit/compilation-unit.h>
#include <jit/tree-node.h>
#include <vm/string.h>
#include <limits.h>

#define BC_OFFSET_UNKNOWN ULONG_MAX

unsigned long native_ptr_to_bytecode_offset(struct compilation_unit *cu,
					    unsigned char *native_ptr);
void print_bytecode_offset(unsigned long bc_offset, struct string *str);
void tree_patch_bc_offset(struct tree_node *node, unsigned long bc_offset);
bool all_insn_have_bytecode_offset(struct compilation_unit *cu);
int bytecode_offset_to_line_no(struct methodblock *mb, unsigned long bc_offset);

#endif
