RUNNER=vm-test-runner
SUITE=vm-test-suite.c

TOPLEVEL_OBJS :=			\
	arch/$(ARCH)/backtrace.o	\
	lib/bitset.o			\
	lib/buffer.o			\
	lib/list.o			\
	lib/radix-tree.o		\
	lib/string.o			\
	vm/bytecode.o			\
	vm/bytecodes.o			\
	vm/die.o			\
	vm/natives.o			\
	vm/stack.o			\
	vm/types.o			\
	vm/zalloc.o			\
	test/libharness/libharness.o	\
	test/vm/stack-trace-stub.o

TEST_OBJS :=				\
	bitset-test.o			\
	buffer-test.o			\
	bytecodes-test.o		\
	list-test.o			\
	natives-test.o			\
	radix-tree-test.o		\
	stack-test.o			\
	string-test.o			\
	types-test.o

CFLAGS += -I ../../arch/mmix/include

include ../../scripts/build/test.mk
