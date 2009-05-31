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
DEFAULT_CFLAGS	:= -rdynamic -g -Wall -Wundef -Wsign-compare -Os -std=gnu99
INCLUDE		:= -I. -I../../arch/$(ARCH)/include -I../libharness -I../jit -I../../include -I../../cafebabe/include -I../include -I../../jit/glib -include $(ARCH_CONFIG) 
MONOBURG	:= ../../monoburg/monoburg

RUNNER=x86-test-runner
SUITE=x86-test-suite.c

OBJS = \
	../libharness/libharness.o \
	../jamvm/alloc-stub.o \
	../jamvm/cast-stub.o \
	../jamvm/lock.o \
	../../arch/x86/backtrace.o \
	../../vm/buffer.o \
	../../vm/list.o \
	../../vm/stack.o \
	../../vm/string.o \
	../../vm/types.o \
	../../vm/zalloc.o \
	../../vm/class.o \
	../../jit/alloc.o \
	../../jit/basic-block.o \
	../../jit/compilation-unit.o \
	../../jit/emit.o \
	../../jit/emulate.o \
	../../jit/expression.o \
	../../jit/interval.o \
	../../jit/stack-slot.o \
	../../jit/statement.o \
	../../jit/tree-printer.o \
	../../jit/fixup-site.o \
	../../jit/bc-offset-mapping.o \
	../../arch/x86/emit-code$(ARCH_POSTFIX).o \
	../../arch/x86/instruction.o \
	../../arch/x86/insn-selector$(ARCH_POSTFIX).o \
	../../arch/x86/stack-frame.o \
	../../arch/x86/use-def.o \
	$(TESTS)

TESTS = \
	emit-code-test$(ARCH_POSTFIX).o \
	insn-selector-test$(ARCH_POSTFIX).o \
	stack-frame-test$(ARCH_POSTFIX).o \
	use-def-test$(ARCH_POSTFIX).o

../../arch/x86/insn-selector.c: FORCE
	$(E) "  MONOBURG" $@
	$(Q) $(MONOBURG) -p -e ../../arch/x86/insn-selector.brg > $@

include ../../scripts/build/test.mk
