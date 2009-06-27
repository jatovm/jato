RUNNER=vm-test-runner
SUITE=vm-test-suite.c

OBJS =					\
	../../arch/$(ARCH)/backtrace.o	\
	../../lib/bitset.o		\
	../../lib/buffer.o		\
	../../lib/list.o		\
	../../lib/list.o		\
	../../lib/radix-tree.o		\
	../../lib/string.o		\
	../../vm/bytecode.o		\
	../../vm/bytecodes.o		\
	../../vm/natives.o		\
	../../vm/stack.o		\
	../../vm/types.o		\
	../../vm/zalloc.o		\
	../libharness/libharness.o	\
	bitset-test.o			\
	buffer-test.o			\
	bytecodes-test.o		\
	list-test.o			\
	natives-test.o			\
	radix-tree-test.o		\
	stack-test.o			\
	string-test.o			\
	types-test.o

include ../../scripts/build/test.mk
