RUNNER	= ppc-test-runner
SUITE	= ppc-test-suite.c

TOPLEVEL_OBJS	+= sys/$(SYS)-$(ARCH)/backtrace.o
TOPLEVEL_OBJS	+= arch/ppc/encode.o
TOPLEVEL_OBJS	+= arch/ppc/init.o
TOPLEVEL_OBJS	+= arch/ppc/instruction.o
TOPLEVEL_OBJS	+= arch/ppc/stack-frame.o
TOPLEVEL_OBJS	+= jit/stack-slot.o
TOPLEVEL_OBJS	+= jit/text.o
TOPLEVEL_OBJS	+= lib/buffer.o
TOPLEVEL_OBJS	+= lib/string.o
TOPLEVEL_OBJS	+= test/unit/libharness/libharness.o
TOPLEVEL_OBJS	+= test/unit/vm/stack-trace-stub.o
TOPLEVEL_OBJS	+= test/unit/vm/trace-stub.o
TOPLEVEL_OBJS	+= vm/die.o
TOPLEVEL_OBJS	+= vm/zalloc.o
TOPLEVEL_OBJS	+= lib/bitset.o

TEST_OBJS	+= encode-test.o

include ../../../scripts/build/test.mk
