RUNNER=vm-test-runner
SUITE=vm-test-suite.c

TOPLEVEL_OBJS :=			\
	sys/$(SYS)-$(ARCH)/backtrace.o	\
	lib/bitset.o			\
	lib/buffer.o			\
	lib/list.o			\
	lib/parse.o			\
	lib/pqueue.o			\
	lib/radix-tree.o		\
	lib/stack.o			\
	lib/string.o			\
	test/unit/jit/trace-stub.o	\
	test/unit/jit/exception-stub.o	\
	test/unit/libharness/libharness.o\
	test/unit/vm/class-stub.o	\
	test/unit/vm/stack-trace-stub.o	\
	test/unit/vm/preload-stub.o	\
	vm/bytecode.o			\
	vm/die.o			\
	vm/natives.o			\
	vm/trace.o			\
	vm/types.o			\
	vm/verifier.o			\
	vm/zalloc.o			\
	test/unit/vm/thread-stub.o

TEST_OBJS :=				\
	bitset-test.o			\
	buffer-test.o			\
	bytecodes-test.o		\
	list-test.o			\
	natives-test.o			\
	verifier-test.o			\
	parse-test.o			\
	pqueue-test.o			\
	radix-tree-test.o		\
	stack-test.o			\
	string-test.o			\
	types-test.o

CFLAGS += -I ../../../arch/mmix/include

include ../../../scripts/build/test.mk
