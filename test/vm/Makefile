RUNNER=vm-test-runner
SUITE=vm-test-suite.c

OBJS = \
	../libharness/libharness.o \
	../../arch/$(ARCH)/backtrace.o \
	../../vm/bitset.o \
	../../vm/buffer.o \
	../../vm/bytecode.o \
	../../vm/bytecodes.o \
	../../vm/list.o \
	../../vm/natives.o \
	../../vm/stack.o \
	../../vm/string.o \
	../../vm/zalloc.o \
	../../vm/types.o \
	bitset-test.o \
	buffer-test.o \
	bytecodes-test.o \
	list-test.o \
	natives-test.o \
	stack-test.o \
	string-test.o \
	types-test.o

include ../../scripts/build/test.mk
