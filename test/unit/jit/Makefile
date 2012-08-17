RUNNER=jit-test-runner
SUITE=jit-test-suite.c

TOPLEVEL_OBJS := \
	sys/$(SYS)-$(ARCH)/backtrace.o \
	cafebabe/attribute_array.o \
	cafebabe/attribute_info.o \
	cafebabe/class.o \
	cafebabe/constant_pool.o \
	cafebabe/error.o \
	cafebabe/field_info.o \
	cafebabe/method_info.o \
	cafebabe/source_file_attribute.o \
	cafebabe/stream.o \
	jit/args.o \
	jit/basic-block.o \
	jit/bc-offset-mapping.o \
	jit/cfg-analyzer.o \
	jit/compilation-unit.o \
	jit/constant-pool.o \
	jit/cu-mapping.o \
	jit/dominance.o \
	jit/exception.o \
	jit/expression.o \
	jit/fixup-site.o \
	jit/interval.o \
	jit/linear-scan.o \
	jit/liveness.o \
	jit/method.o \
	jit/spill-reload.o \
	jit/stack-slot.o \
	jit/statement.o \
	jit/text.o \
	jit/tree-node.o \
	jit/tree-printer.o \
	jit/subroutine.o \
	jit/pc-map.o \
	jit/constant_pool.o \
	lib/arena.o \
	lib/bitset.o \
	lib/buffer.o \
	lib/compile-lock.o \
	lib/guard-page.o \
	lib/hash-map.o \
	lib/list.o \
	lib/pqueue.o \
	lib/radix-tree.o \
	lib/stack.o \
	lib/string.o \
	lib/symbol.o \
	vm/call.o \
	vm/bytecode.o \
	vm/die.o \
	vm/trace.o \
	vm/types.o \
	vm/zalloc.o \
	arch/mmix/args.o \
	arch/mmix/call.o \
	arch/mmix/instruction.o \
	arch/mmix/register.o \
	arch/mmix/stack-frame.o \
	arch/mmix/use-def.o \
	test/unit/libharness/libharness.o \
	test/unit/vm/class-stub.o \
	test/unit/vm/classloader-stub.o \
	test/unit/vm/gc-stub.o \
	test/unit/vm/object-stub.o \
	test/unit/vm/preload-stub.o \
	test/unit/vm/jni-stub.o \
	test/unit/vm/stack-trace-stub.o \
	test/unit/vm/thread-stub.o \
	test/unit/jit/trace-stub.o

TEST_OBJS := \
	args-test-utils.o \
	basic-block-test.o \
	bc-test-utils.o \
	cfg-analyzer-test.o \
	compilation-unit-test.o \
	dominance-test.o \
	expression-test.o \
	linear-scan-test.o \
	live-range-test.o \
	liveness-test.o \
	spill-reload-test.o \
	stack-slot-test.o \
	tree-printer-test.o

include ../../../scripts/build/test.mk

DEFAULT_CFLAGS += -I../../../arch/mmix/include -std=gnu99
