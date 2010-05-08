RUNNER=vm-test-runner
SUITE=vm-test-suite.c

TOPLEVEL_OBJS :=			\
	arch/$(ARCH)/backtrace.o	\
	lib/bitset.o			\
	lib/buffer.o			\
	lib/list.o			\
	lib/parse.o			\
	lib/pqueue.o			\
	lib/radix-tree.o		\
	lib/stack.o			\
	lib/string.o			\
	test/jit/trace-stub.o		\
	test/libharness/libharness.o	\
	test/vm/class-stub.o		\
	test/vm/stack-trace-stub.o	\
	vm/bytecode.o			\
	vm/die.o			\
	vm/natives.o			\
	vm/trace.o			\
	vm/types.o			\
	vm/zalloc.o			\
	test/vm/thread-stub.o

TEST_OBJS :=				\
	bitset-test.o			\
	buffer-test.o			\
	bytecodes-test.o		\
	list-test.o			\
	natives-test.o			\
	parse-test.o			\
	pqueue-test.o			\
	radix-tree-test.o		\
	stack-test.o			\
	string-test.o			\
	types-test.o

CFLAGS += -I ../../arch/mmix/include

include ../../scripts/build/test.mk
