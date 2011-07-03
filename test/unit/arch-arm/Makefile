RUNNER	= arm-test-runner
SUITE	= arm-test-suite.c

TOPLEVEL_OBJS	+= sys/$(SYS)-$(ARCH)/backtrace.o
TOPLEVEL_OBJS	+= lib/buffer.o
TOPLEVEL_OBJS	+= lib/string.o
TOPLEVEL_OBJS	+= lib/radix-tree.o
TOPLEVEL_OBJS	+= lib/arena.o
TOPLEVEL_OBJS	+= lib/stack.o
TOPLEVEL_OBJS	+= lib/compile-lock.o
TOPLEVEL_OBJS	+= jit/compilation-unit.o
TOPLEVEL_OBJS	+= jit/basic-block.o
TOPLEVEL_OBJS	+= jit/statement.o
TOPLEVEL_OBJS	+= jit/expression.o
TOPLEVEL_OBJS	+= jit/fixup-site.o
TOPLEVEL_OBJS	+= jit/interval.o
TOPLEVEL_OBJS	+= jit/stack-slot.o
TOPLEVEL_OBJS	+= jit/text.o
TOPLEVEL_OBJS	+= arch/arm/instruction.o
TOPLEVEL_OBJS	+= arch/arm/insn-selector.o
TOPLEVEL_OBJS	+= arch/arm/constant_pool.o
TOPLEVEL_OBJS	+= test/unit/libharness/libharness.o
TOPLEVEL_OBJS	+= test/unit/vm/stack-trace-stub.o
TOPLEVEL_OBJS	+= test/unit/vm/trace-stub.o
TOPLEVEL_OBJS	+= test/unit/jit/bytecode-to-ir-stub.o
TOPLEVEL_OBJS	+= vm/die.o
TOPLEVEL_OBJS	+= vm/bytecode.o
TOPLEVEL_OBJS	+= vm/zalloc.o
TOPLEVEL_OBJS	+= lib/bitset.o

TEST_OBJS	+= compilation-unit-stub.o insn-selector-test.o

include ../../../scripts/build/test.mk
