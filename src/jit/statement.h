#ifndef __STATEMENT_H
#define __STATEMENT_H

#include <constant.h>
#include <stddef.h>

enum statement_type {
	STMT_NOP,
	STMT_ASSIGN
};

struct statement {
	enum statement_type type;
	struct constant operand;
};

extern struct statement *stmt_from_bytecode(char *code, size_t count);

#endif
