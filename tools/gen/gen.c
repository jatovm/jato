#include <stdio.h>
#include <stdlib.h>

static const unsigned int nr_variables = 10;
static const unsigned int nr_statements = 100;
static const unsigned int nr_expression_levels = 3;

static void gen_expr(unsigned int level);

static void gen_variable_expr(void)
{
	printf("v%d", rand() % nr_variables);
}

static void gen_arith_expr(unsigned int level)
{
	static const char *operators[] = {"+", "-", "*"};

	printf("(");
	gen_expr(level - 1);
	printf(" %s ", operators[rand() % (sizeof(operators) / sizeof(*operators))]);
	gen_expr(level - 1);
	printf(")");
}

static void gen_test_expr(unsigned int level)
{
	static const char *operators[] = {"==", "!=", "<", ">", "<=", ">="};

	printf("(");
	gen_expr(level - 1);
	printf(" %s ", operators[rand() % (sizeof(operators) / sizeof(*operators))]);
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

int main(int argc, char *argv[])
{
	printf("class Foo {\n");
	printf("public static void main(String args[]) {\n");
	for (unsigned int i = 0; i < nr_variables; ++i)
		printf("int v%d = %d;\n", i, rand());
	for (unsigned int i = 0; i < nr_statements; ++i) {
		printf("v%d = ", rand() % nr_variables);
		gen_expr(nr_expression_levels);
		printf(";\n");
	}
	printf("System.out.println(");
	for (unsigned int i = 0; i < nr_variables; ++i)
		printf("v%d%s", i, i < nr_variables - 1 ? " + " : "");
	printf(");\n");
	printf("}\n");
	printf("}\n");
	return 0;
}
