RUNNER	= x86-test-runner
SUITE	= x86-test-suite.c

TOPLEVEL_OBJS	+= sys/$(SYS)-$(ARCH)/backtrace.o
TOPLEVEL_OBJS	+= arch/x86/encode.o
TOPLEVEL_OBJS	+= arch/x86/init.o
TOPLEVEL_OBJS	+= arch/x86/instruction.o
TOPLEVEL_OBJS	+= arch/x86/stack-frame.o
TOPLEVEL_OBJS	+= jit/stack-slot.o
TOPLEVEL_OBJS	+= jit/text.o
TOPLEVEL_OBJS	+= lib/buffer.o
TOPLEVEL_OBJS	+= lib/string.o
TOPLEVEL_OBJS	+= test/libharness/libharness.o
TOPLEVEL_OBJS	+= test/vm/stack-trace-stub.o
TOPLEVEL_OBJS	+= test/vm/trace-stub.o
TOPLEVEL_OBJS	+= vm/die.o
TOPLEVEL_OBJS	+= vm/zalloc.o

TEST_OBJS	+= encode-test.o trampoline-stub.o

include ../../scripts/build/test.mk
