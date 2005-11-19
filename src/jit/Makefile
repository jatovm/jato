TEST_EXEC=run_all_tests
ALLTESTS=all_test.c

all: test

tags:
	exuberant-ctags -R

test:
	sh make-tests.sh > $(ALLTESTS)
	gcc -rdynamic -g -Wall -I. -I.. $(ALLTESTS) CuTest.c operand-stack-test.c statement-test.c bytecode-conv.c backtrace.c -o $(TEST_EXEC)
	./$(TEST_EXEC)

clean:
	rm -rf $(TEST_EXEC) $(ALLTESTS) tags
