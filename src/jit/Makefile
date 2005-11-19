TEST_EXEC=run_all_tests
ALLTESTS=all_test.c

CC = gcc
CCFLAGS = -rdynamic -g -Wall 
INCLUDE = -I. -I..
OBJS = bytecode-conv.o
TESTS = operand-stack-test.o statement-test.o
HARNESS = CuTest.o backtrace.o

%.o: %.c
	$(CC) $(CCFLAGS) $(INCLUDE) -c $< -o $@

all: test

$(OBJS):

$(TESTS):

$(HARNESS):

tags:
	exuberant-ctags -R

gen-alltests:
	sh make-tests.sh > $(ALLTESTS)

test: $(OBJS) $(TESTS) $(HARNESS) gen-alltests
	$(CC) $(CCFLAGS) $(INCLUDE) $(ALLTESTS) $(OBJS) $(TESTS) $(HARNESS) -o $(TEST_EXEC)
	./$(TEST_EXEC)

clean:
	rm -rf $(TEST_EXEC) $(ALLTESTS) $(OBJS) tags
