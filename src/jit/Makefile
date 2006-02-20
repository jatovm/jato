TESTRUNNER=test-runner
TEST_SUITE=test-suite.c

CC = gcc
CCFLAGS = -rdynamic -g -Wall -Os
INCLUDE = -I. -I..
OBJS = basic-block.o bitmap.o bytecodes.o bytecode-cfg-builder.o bytecode-to-ir.o compilation-unit.o statement.o expression.o
TESTS = basic-block-test.o bitmap-test.o bytecodes-test.o bytecode-cfg-builder-test.o bytecode-to-ir-test.o stack-test.o
HARNESS = libharness.o backtrace.o

%.o: %.c
	$(CC) $(CCFLAGS) $(INCLUDE) -c $< -o $@

all: test

$(OBJS):

$(TESTS):

$(HARNESS):

tags:
	exuberant-ctags -R

gen-suite:
	sh make-tests.sh > $(TEST_SUITE)

test: $(OBJS) $(TESTS) $(HARNESS) gen-suite
	$(CC) $(CCFLAGS) $(INCLUDE) $(TEST_SUITE) $(OBJS) $(TESTS) $(HARNESS) -o $(TESTRUNNER)
	./$(TESTRUNNER)

clean:
	rm -rf $(TESTRUNNER) $(TEST_SUITE) $(OBJS) $(HARNESS) $(TESTS) tags
