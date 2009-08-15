RUNNER=jit-test-runner
SUITE=jit-test-suite.c

TOPLEVEL_OBJS := \
	arch/$(ARCH)/backtrace.o \
	cafebabe/src/cafebabe/attribute_array.o \
	cafebabe/src/cafebabe/attribute_info.o \
	cafebabe/src/cafebabe/class.o \
	cafebabe/src/cafebabe/constant_pool.o \
	cafebabe/src/cafebabe/error.o \
	cafebabe/src/cafebabe/field_info.o \
	cafebabe/src/cafebabe/method_info.o \
	cafebabe/src/cafebabe/source_file_attribute.o \
	cafebabe/src/cafebabe/stream.o \
	jit/args.o \
	jit/arithmetic-bc.o \
	jit/basic-block.o \
	jit/bc-offset-mapping.o \
	jit/branch-bc.o \
	jit/bytecode-to-ir.o \
	jit/cfg-analyzer.o \
	jit/compilation-unit.o \
	jit/cu-mapping.o \
	jit/exception-bc.o \
	jit/exception.o \
	jit/expression.o \
	jit/interval.o \
	jit/invoke-bc.o \
	jit/linear-scan.o \
	jit/liveness.o \
	jit/load-store-bc.o \
	jit/method.o \
	jit/nop-bc.o \
	jit/object-bc.o \
	jit/ostack-bc.o \
	jit/spill-reload.o \
	jit/stack-slot.o \
	jit/statement.o \
	jit/switch-bc.o \
	jit/text.o \
	jit/tree-node.o \
	jit/tree-printer.o \
	jit/typeconv-bc.o \
	jit/subroutine.o \
	jit/pc-map.o \
	jit/wide-bc.o \
	lib/bitset.o \
	lib/buffer.o \
	lib/list.o \
	lib/guard-page.o \
	lib/pqueue.o \
	lib/radix-tree.o \
	lib/string.o \
	vm/call.o \
	vm/bytecode.o \
	vm/bytecodes.o \
	vm/die.o \
	vm/stack.o \
	vm/trace.o \
	vm/types.o \
	vm/zalloc.o \
	arch/mmix/call.o \
	arch/mmix/instruction.o \
	arch/mmix/register.o \
	arch/mmix/stack-frame.o \
	arch/mmix/use-def.o \
	test/libharness/libharness.o \
	test/vm/class-stub.o \
	test/vm/classloader-stub.o \
	test/vm/gc-stub.o \
	test/vm/object-stub.o \
	test/vm/preload-stub.o \
	test/vm/jni-stub.o \
	test/vm/stack-trace-stub.o \
	test/vm/thread-stub.o \
	test/jit/trace-stub.o

TEST_OBJS := \
	args-test-utils.o \
	arithmetic-bc-test.o \
	basic-block-test.o \
	bc-test-utils.o \
	branch-bc-test.o \
	bytecode-to-ir-test.o \
	cfg-analyzer-test.o \
	compilation-unit-test.o \
	expression-test.o \
	invoke-bc-test.o \
	linear-scan-test.o \
	live-range-test.o \
	liveness-test.o \
	load-store-bc-test.o \
	object-bc-test.o \
	ostack-bc-test.o \
	spill-reload-test.o \
	stack-slot-test.o \
	tree-printer-test.o \
	typeconv-bc-test.o

DEFAULT_CFLAGS += -I../../arch/mmix/include -std=gnu99

include ../../scripts/build/test.mk
