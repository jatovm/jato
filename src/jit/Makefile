TESTRUNNER=test-runner
TEST_SUITE=test-suite.c

CC = gcc
CCFLAGS = -rdynamic -g -Wall 
INCLUDE = -I. -I..
OBJS = bytecode-conv.o
TESTS = bytecode-conv-test.o operand-stack-test.o
HARNESS = CuTest.o backtrace.o

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
