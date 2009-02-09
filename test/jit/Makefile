RUNNER=jit-test-runner
SUITE=jit-test-suite.c

OBJS = \
	../../arch/$(ARCH)/backtrace.o \
	../../vm/bitset.o \
	../../vm/buffer.o \
	../../vm/bytecode.o \
	../../vm/bytecodes.o \
	../../vm/stack.o \
	../../vm/string.o \
	../../vm/types.o \
	../../vm/zalloc.o \
	../../jit/arithmetic-bc.o \
	../../jit/basic-block.o \
	../../jit/branch-bc.o \
	../../jit/bytecode-to-ir.o \
	../../jit/cfg-analyzer.o \
	../../jit/compilation-unit.o \
	../../jit/expression.o \
	../../jit/interval.o \
	../../jit/invoke-bc.o \
	../../jit/linear-scan.o \
	../../jit/liveness.o \
	../../jit/load-store-bc.o \
	../../jit/exception-bc.o \
	../../jit/object-bc.o \
	../../jit/ostack-bc.o \
	../../jit/stack-slot.o \
	../../jit/statement.o \
	../../jit/spill-reload.o \
	../../jit/typeconv-bc.o \
	../../jit/tree-printer.o \
	../../jit/args.o \
	../libharness/libharness.o \
	../jamvm/alloc-stub.o \
	../jamvm/resolve-stub.o \
	../arch-mmix/instruction.o \
	../arch-mmix/use-def.o \
	arithmetic-bc-test.o \
	basic-block-test.o \
	bc-test-utils.o \
	branch-bc-test.o \
	bytecode-to-ir-test.o \
	cfg-analyzer-test.o \
	compilation-unit-test.o \
	expression-test.o \
	interval-test.o \
	args-test-utils.o \
	invoke-bc-test.o \
	linear-scan-test.o \
	liveness-test.o \
	live-range-test.o \
	load-store-bc-test.o \
	object-bc-test.o \
	ostack-bc-test.o \
	stack-slot-test.o \
	spill-reload-test.o \
	tree-printer-test.o \
	typeconv-bc-test.o

include ../scripts/test.mk
