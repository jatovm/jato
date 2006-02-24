TESTRUNNER=test-runner
TEST_SUITE=test-suite.c

CC = gcc
CCFLAGS = -rdynamic -g -Wall -Wundef -Wsign-compare -Werror -Os -std=gnu99
INCLUDE = -I. -I.. -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include
LIBS = -lglib-2.0
OBJS = basic-block.o bitmap.o bytecodes.o bytecode-cfg-builder.o bytecode-to-ir.o compilation-unit.o instruction.o insn-selector.o statement.o expression.o
TESTS = basic-block-test.o bitmap-test.o bytecodes-test.o bytecode-cfg-builder-test.o bytecode-to-ir-test.o insn-selector-test.o list-test.o stack-test.o
HARNESS = libharness.o backtrace.o

%.o: %.c
	$(CC) $(CCFLAGS) $(INCLUDE) -c $< -o $@

all: test

$(OBJS):

$(TESTS):

$(HARNESS):

insn-selector.c: insn-selector.brg
	../../monoburg/monoburg -e insn-selector.brg > insn-selector.c

tags:
	exuberant-ctags -R

gen-suite:
	sh make-tests.sh > $(TEST_SUITE)

test: $(OBJS) $(TESTS) $(HARNESS) gen-suite
	$(CC) $(CCFLAGS) $(INCLUDE) $(TEST_SUITE) $(LIBS) $(OBJS) $(TESTS) $(HARNESS) -o $(TESTRUNNER)
	./$(TESTRUNNER)

clean:
	rm -rf insn-selector.c $(TESTRUNNER) $(TEST_SUITE) *.o tags
