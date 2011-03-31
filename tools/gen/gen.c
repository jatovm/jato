#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static const unsigned int nr_variables = 10;
static const unsigned int nr_statements = 120;
static const unsigned int nr_expression_levels = 3;
static const unsigned int nr_statement_levels = 2;
static const unsigned int nr_branches = 2;

enum type {
	BYTE, INT, LONG, FLOAT, DOUBLE,
	NR_TYPES,
};

static const char *types[NR_TYPES] = {
	"byte", "int", "long", "float", "double"
};

static enum type variable_type[nr_variables];

static void gen_expr(unsigned int level);

static void gen_variable_expr(void)
{
	printf("v%d", rand() % nr_variables);
}

static void gen_arith_expr(unsigned int level)
{
	static const char *operators[] = {"+", "-", "*"};

	unsigned int type = rand() % (sizeof(types) / sizeof(*types));

	printf("((%s) ", types[type]);
	gen_expr(level - 1);
	printf(" %s (%s) ", operators[rand() % (sizeof(operators) / sizeof(*operators))], types[type]);
	gen_expr(level - 1);
	printf(")");
}

static void gen_test_expr(unsigned int level)
{
	static const char *operators[] = {"==", "!=", "<", ">", "<=", ">="};

	unsigned int type = rand() % (sizeof(types) / sizeof(*types));

	if (rand() % 2)
		printf("!");

	printf("((%s) ", types[type]);
	gen_expr(level - 1);
	printf(" %s (%s) ", operators[rand() % (sizeof(operators) / sizeof(*operators))], types[type]);
	gen_expr(level - 1);
	printf(")");
}

static void gen_expr(unsigned int level)
{
	if (!level || rand() % 100 < 5) {
		gen_variable_expr();
		return;
	}

	if (rand() % 100 < 20) {
		printf("(");
		gen_test_expr(level);
		printf(" ? ");
		gen_expr(level);
		printf(" : ");
		gen_expr(level);
		printf(")");
		return;
	}

	gen_arith_expr(level);
}

static void gen_decl(unsigned int var)
{
	variable_type[var] = (type) (rand() % NR_TYPES);

	printf("%s v%d = ", types[variable_type[var]], var);
	switch (variable_type[var]) {
	case 0:
		printf("%d", rand() % 256 - 128);
		break;
	case 1:
		printf("%d", rand());
		break;
	case 2:
		printf("%ld", 1L * rand());
		break;
	case 3:
		printf("%ff", 1.0f * rand());
		break;
	case 4:
		printf("%f", 1.0 * rand());
		break;
	default:
		assert(false);
	}

	printf(";\n");
}

static void gen_stmt(unsigned int level);

static void gen_assignment_stmt()
{
	unsigned int var = rand() % nr_variables;
	printf("v%d = (%s) ", var, types[variable_type[var]]);
	gen_expr(nr_expression_levels);
	printf(";\n");
}

static void gen_if_stmt(unsigned int level)
{
	unsigned int n = rand() % nr_branches;

	for (unsigned int i = 0; i < n; ++i) {
		printf("if (");
		gen_test_expr(2);
		printf(") {\n");
		gen_stmt(level - 1);
		printf("} else ");
	}

	printf("{\n");
	gen_stmt(level - 1);
	printf("}");
}

static void gen_stmt(unsigned int level)
{
	if (!level || rand() % 100 < 5) {
		gen_assignment_stmt();
		return;
	}

	gen_if_stmt(level);
}

int main(int argc, char *argv[])
{
	srand(time(NULL));

	printf("class Test {\n");
	printf("public static void main(String args[]) {\n");
	for (unsigned int i = 0; i < nr_variables; ++i)
		gen_decl(i);
	for (unsigned int i = 0; i < nr_statements; ++i)
		gen_stmt(nr_statement_levels);
	printf("System.out.println(");
	for (unsigned int i = 0; i < nr_variables; ++i)
		printf("v%d%s", i, i < nr_variables - 1 ? " + " : "");
	printf(");\n");
	printf("}\n");
	printf("}\n");
	return 0;
}
