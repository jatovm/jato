ARCH		?= $(shell uname -m | sed -e s/i.86/i386/)
OS		?= $(shell uname -s | tr "[:upper:]" "[:lower:]")

ifeq ($(ARCH),i386)
ARCH		= x86
ARCH_POSTFIX	= _32
endif

ifeq ($(ARCH),x86_64)
ARCH		= x86
ARCH_POSTFIX	= _64
endif

ARCH_CONFIG	:= ../../arch/$(ARCH)/include/arch/config$(ARCH_POSTFIX).h
DEFAULT_CFLAGS	:= -rdynamic -g -Wall -Wundef -Wsign-compare -Os -std=gnu99 -D_GNU_SOURCE
INCLUDE		:= -I. -I../../arch/$(ARCH)/include -I../libharness -I../jit -I../../include -I../../cafebabe/include -I../include -I../../jit/glib -include $(ARCH_CONFIG) 
MONOBURG	:= ../../monoburg/monoburg

RUNNER=x86-test-runner
SUITE=x86-test-suite.c

OBJS = \
	../../arch/x86/backtrace.o \
	../../arch/x86/emit-code.o \
	../../arch/x86/exception.o \
	../../arch/x86/insn-selector.o \
	../../arch/x86/registers$(ARCH_POSTFIX).o \
	../../arch/x86/instruction.o \
	../../arch/x86/stack-frame.o \
	../../arch/x86/thread.o \
	../../arch/x86/unwind$(ARCH_POSTFIX).o \
	../../arch/x86/use-def.o \
	../../cafebabe/src/cafebabe/attribute_array.o \
	../../cafebabe/src/cafebabe/attribute_info.o \
	../../cafebabe/src/cafebabe/class.o \
	../../cafebabe/src/cafebabe/code_attribute.o \
	../../cafebabe/src/cafebabe/constant_pool.o \
	../../cafebabe/src/cafebabe/constant_value_attribute.o \
	../../cafebabe/src/cafebabe/error.o \
	../../cafebabe/src/cafebabe/field_info.o \
	../../cafebabe/src/cafebabe/line_number_table_attribute.o \
	../../cafebabe/src/cafebabe/method_info.o \
	../../cafebabe/src/cafebabe/source_file_attribute.o \
	../../cafebabe/src/cafebabe/stream.o \
	../../jit/basic-block.o \
	../../jit/bc-offset-mapping.o \
	../../jit/compilation-unit.o \
	../../jit/cu-mapping.o \
	../../jit/emit.o \
	../../jit/emulate.o \
	../../jit/exception.o \
	../../jit/expression.o \
	../../jit/fixup-site.o \
	../../jit/interval.o \
	../../jit/method.o \
	../../jit/stack-slot.o \
	../../jit/statement.o \
	../../jit/text.o \
	../../jit/tree-node.o \
	../../jit/tree-printer.o \
	../../jit/vtable.o \
	../../lib/buffer.o \
	../../lib/list.o \
	../../lib/radix-tree.o \
	../../lib/string.o \
	../../vm/class.o \
	../../vm/die.o \
	../../vm/field.o \
	../../vm/guard-page.o \
	../../vm/itable.o \
	../../vm/method.o \
	../../vm/object.o \
	../../vm/stack.o \
	../../vm/static.o \
	../../vm/types.o \
	../../vm/utf8.o \
	../../vm/zalloc.o \
	../../vm/fault-inject.o \
	../../vm/natives.o \
	../jamvm/alloc-stub.o \
	../jamvm/cast-stub.o \
	../jamvm/lock.o \
	../jit/trampoline-stub.o \
	../jit/trace-stub.o \
	../libharness/libharness.o \
	../vm/classloader-stub.o \
	../vm/preload-stub.o \
	../vm/signal-stub.o \
	$(TESTS)

TESTS = \
	emit-code-test$(ARCH_POSTFIX).o \
	stack-frame-test$(ARCH_POSTFIX).o \
	use-def-test$(ARCH_POSTFIX).o

../../arch/x86/insn-selector.c: FORCE
	$(E) "  MONOBURG" $@
	$(Q) $(MONOBURG) -p -e ../../arch/x86/insn-selector.brg > $@

include ../../scripts/build/test.mk
