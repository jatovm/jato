CLASSPATH_CONFIG = tools/classpath-config

JAMVM_INSTALL_DIR	:= /usr/local
CLASSPATH_INSTALL_DIR	?= $(shell ./tools/classpath-config)

GLIBJ		= $(CLASSPATH_INSTALL_DIR)/share/classpath/glibj.zip

BUILD_ARCH	:= $(shell uname -m | sed -e s/i.86/i386/)
ARCH		:= $(BUILD_ARCH)
JAMVM_ARCH	:= $(shell echo "$(ARCH)" | sed -e s/ppc/powerpc/)
OS		:= $(shell uname -s | tr "[:upper:]" "[:lower:]")

ifneq ($(ARCH),$(BUILD_ARCH))
TEST		=
else
TEST		= test
endif

ifeq ($(ARCH),i386)
override ARCH	= x86
ARCH_POSTFIX	= _32
ifeq ($(BUILD_ARCH),x86_64)
ARCH_CFLAGS	+= -m32
TEST		= test
endif
endif

ifeq ($(ARCH),x86_64)
override ARCH	= x86
ARCH_POSTFIX	= _64
endif

ifeq ($(ARCH),ppc)
override ARCH	= ppc
ARCH_POSTFIX	= _32
endif

export ARCH_CFLAGS

ARCH_CONFIG=arch/$(ARCH)/include/arch/config$(ARCH_POSTFIX).h
LINKER_SCRIPT=arch/$(ARCH)/jato$(ARCH_POSTFIX).ld

# Make the build silent by default
V =

PROGRAM		:= jato

include arch/$(ARCH)/Makefile$(ARCH_POSTFIX)

JIT_OBJS = \
	jit/args.o		\
	jit/arithmetic-bc.o	\
	jit/basic-block.o	\
	jit/bc-offset-mapping.o \
	jit/branch-bc.o		\
	jit/bytecode-to-ir.o	\
	jit/cfg-analyzer.o	\
	jit/compilation-unit.o	\
	jit/compiler.o		\
	jit/cu-mapping.o	\
	jit/disass-common.o	\
	jit/emit.o		\
	jit/emulate.o		\
	jit/exception-bc.o	\
	jit/exception.o 	\
	jit/expression.o	\
	jit/fixup-site.o	\
	jit/interval.o		\
	jit/invoke-bc.o		\
	jit/linear-scan.o	\
	jit/liveness.o		\
	jit/load-store-bc.o	\
	jit/method.o		\
	jit/nop-bc.o		\
	jit/object-bc.o		\
	jit/ostack-bc.o		\
	jit/perf-map.o		\
	jit/spill-reload.o	\
	jit/stack-slot.o	\
	jit/statement.o		\
	jit/text.o		\
	jit/trace-jit.o		\
	jit/trampoline.o	\
	jit/tree-node.o		\
	jit/tree-printer.o	\
	jit/typeconv-bc.o	\
	jit/vtable.o		\

VM_OBJS = \
	vm/bytecode.o		\
	vm/bytecodes.o		\
	vm/class.o		\
	vm/classloader.o	\
	vm/debug-dump.o		\
	vm/die.o		\
	vm/field.o		\
	vm/guard-page.o		\
	vm/jato.o		\
	vm/method.o		\
	vm/natives.o		\
	vm/object.o		\
	vm/resolve.o		\
	vm/signal.o		\
	vm/stack.o		\
	vm/stack-trace.o	\
	vm/static.o		\
	vm/types.o		\
	vm/utf8.o		\
	vm/zalloc.o		\
	vm/preload.o		\
	vm/fault-inject.o

LIB_OBJS = \
	lib/bitset.o		\
	lib/buffer.o		\
	lib/list.o		\
	lib/radix-tree.o	\
	lib/string.o

JAMVM_OBJS =

CAFEBABE_OBJS := \
	attribute_array.o		\
	attribute_info.o		\
	class.o				\
	code_attribute.o		\
	constant_value_attribute.o	\
	constant_pool.o			\
	error.o				\
	field_info.o			\
	method_info.o			\
	stream.o

CAFEBABE_OBJS := $(addprefix cafebabe/src/cafebabe/,$(CAFEBABE_OBJS))

LIBHARNESS_OBJS = \
	test/libharness/libharness.o

JATO_OBJS = $(ARCH_OBJS) $(JIT_OBJS) $(VM_OBJS) $(LIB_OBJS) $(CAFEBABE_OBJS)

OBJS = $(JAMVM_OBJS) $(JATO_OBJS)

RUNTIME_CLASSES =

CC		:= gcc
MONOBURG	:= ./monoburg/monoburg
JAVAC		:= ecj
JAVAC_OPTS	:= -encoding utf-8

DEFAULT_CFLAGS	+= $(ARCH_CFLAGS) -g -Wall -rdynamic -std=gnu99 -D_GNU_SOURCE

# XXX: Temporary hack -Vegard
DEFAULT_CFLAGS	+= -DNOT_IMPLEMENTED='fprintf(stderr, "%s:%d: warning: %s not implemented\n", __FILE__, __LINE__, __func__)'

WARNINGS	= -Wsign-compare -Wundef
DEFAULT_CFLAGS	+= $(WARNINGS)

OPTIMIZATIONS	+= -Os
DEFAULT_CFLAGS	+= $(OPTIMIZATIONS)

INCLUDES	= -Iinclude -Iarch/$(ARCH)/include -Ijit -Ijit/glib -Icafebabe/include -include $(ARCH_CONFIG)
DEFAULT_CFLAGS	+= $(INCLUDES)

DEFINES = -DINSTALL_DIR=\"$(JAMVM_INSTALL_DIR)\" -DCLASSPATH_INSTALL_DIR=\"$(CLASSPATH_INSTALL_DIR)\"
DEFAULT_CFLAGS	+= $(DEFINES)

DEFAULT_LIBS	= -lpthread -lm -ldl -lz -lzip -lbfd -lopcodes -liberty $(ARCH_LIBS)

all: $(PROGRAM) $(TEST)
.PHONY: all
.DEFAULT: all

$(CLASSPATH_CONFIG):
	$(E) "  LD      " $@
	$(Q) $(CC) -Wall $(CLASSPATH_CONFIG).c -o $(CLASSPATH_CONFIG)

monoburg:
	$(Q) make -C monoburg/
.PHONY: monoburg

%.o: %.c
	$(E) "  CC      " $@
	$(Q) $(CC) -c $(DEFAULT_CFLAGS) $(CFLAGS) $< -o $@
	$(Q) $(CC) -MM $(DEFAULT_CFLAGS) $(CFLAGS) -MT $@ $*.c -o $*.d

%.o: %.S
	$(E) "  AS      " $@
	$(Q) $(CC) -c $(DEFAULT_CFLAGS) $(CFLAGS) $< -o $@

arch/$(ARCH)/insn-selector$(ARCH_POSTFIX).c: monoburg FORCE
	$(E) "  MONOBURG" $@
	$(Q) $(MONOBURG) -p -e $(@:.c=.brg) > $@

$(PROGRAM): monoburg $(CLASSPATH_CONFIG) compile $(RUNTIME_CLASSES)
	$(E) "  LD      " $@
	$(Q) $(CC) -T $(LINKER_SCRIPT) $(DEFAULT_CFLAGS) $(CFLAGS) $(OBJS) -o $(PROGRAM) $(LIBS) $(DEFAULT_LIBS)

compile: $(OBJS)

test: monoburg
	make -C test/vm/ ARCH=$(ARCH) ARCH_POSTFIX=$(ARCH_POSTFIX) $(TEST)
	make -C test/jit/ ARCH=$(ARCH) ARCH_POSTFIX=$(ARCH_POSTFIX) $(TEST)
	make -C test/arch-$(ARCH)/ ARCH=$(ARCH) ARCH_POSTFIX=$(ARCH_POSTFIX) $(TEST)
.PHONY: test

REGRESSION_TEST_SUITE_CLASSES = \
	regression/jato/internal/VM.class \
	regression/jvm/TestCase.class \
	regression/jvm/ArrayTest.class \
	regression/jvm/ArrayMemberTest.class \
	regression/jvm/BranchTest.class \
	regression/jvm/GetstaticPatchingTest.class \
	regression/jvm/PutstaticPatchingTest.class \
	regression/jvm/InvokestaticPatchingTest.class \
	regression/jvm/ObjectArrayTest.class \
	regression/jvm/ExitStatusIsOneTest.class \
	regression/jvm/ExitStatusIsZeroTest.class \
	regression/jvm/LoadConstantsTest.class \
	regression/jvm/IntegerArithmeticTest.class \
	regression/jvm/FloatArithmeticTest.class \
	regression/jvm/LongArithmeticTest.class \
	regression/jvm/ObjectCreationAndManipulationTest.class \
	regression/jvm/ControlTransferTest.class \
	regression/jvm/SynchronizationTest.class \
	regression/jvm/MethodInvocationAndReturnTest.class \
	regression/jvm/ConversionTest.class \
	regression/jvm/PutstaticTest.class \
	regression/jvm/PutfieldTest.class \
	regression/jvm/StringTest.class \
	regression/jvm/TrampolineBackpatchingTest.class \
	regression/jvm/RegisterAllocatorTortureTest.class \
	regression/jvm/ExceptionsTest.class \
	regression/jvm/FibonacciTest.class \
	regression/jvm/ObjectStackTest.class \
	regression/jvm/PrintTest.class \
	regression/jvm/ArrayExceptionsTest.class \
	regression/jvm/IntegerArithmeticExceptionsTest.class \
	regression/jvm/LongArithmeticExceptionsTest.class \
	regression/jvm/MethodInvocationExceptionsTest.class \
	regression/jvm/ObjectCreationAndManipulationExceptionsTest.class \
	regression/jvm/SynchronizationExceptionsTest.class \
	regression/jvm/ClassExceptionsTest.class

$(REGRESSION_TEST_SUITE_CLASSES): %.class: %.java
	$(E) "  JAVAC   " $@
	$(Q) $(JAVAC) -cp $(GLIBJ):regression $(JAVAC_OPTS) -d regression $<

$(RUNTIME_CLASSES): %.class: %.java
	$(E) "  JAVAC   " $@
	$(Q) $(JAVAC) -cp $(GLIBJ) $(JAVAC_OPTS) -d runtime/classpath $<

lib: $(CLASSPATH_CONFIG)
	make -C lib/ JAVAC=$(JAVAC) GLIBJ=$(GLIBJ)
.PHONY: lib

regression: monoburg $(CLASSPATH_CONFIG) $(PROGRAM) $(REGRESSION_TEST_SUITE_CLASSES)
	$(E) "  REGRESSION"
	$(Q) cd regression && /bin/bash run-suite.sh $(JAVA_OPTS)
.PHONY: regression

clean:
	$(E) "  CLEAN"
	$(Q) - rm -f $(PROGRAM)
	$(Q) - rm -f $(CLASSPATH_CONFIG)
	$(Q) - rm -f $(OBJS)
	$(Q) - rm -f $(OBJS:.o=.d)
	$(Q) - rm -f $(LIBHARNESS_OBJS)
	$(Q) - rm -f $(ARCH_TEST_OBJS)
	$(Q) - rm -f arch/$(ARCH)/insn-selector.c
	$(Q) - rm -f $(PROGRAM)
	$(Q) - rm -f $(ARCH_TEST_SUITE)
	$(Q) - rm -f test-suite.o
	$(Q) - rm -f $(ARCH_TESTRUNNER)
	$(Q) - rm -f $(REGRESSION_TEST_SUITE_CLASSES)
	$(Q) - rm -f $(RUNTIME_CLASSES)
	$(Q) - find regression/ -name "*.class" | xargs rm -f
	$(Q) - find runtime/ -name "*.class" | xargs rm -f
	$(Q) - rm -f tags
	$(Q) - rm -f include/arch
	$(Q) - make -C monoburg/ clean
	$(Q) - make -C test/vm/ clean
	$(Q) - make -C test/jit/ clean
	$(Q) - make -C test/arch-$(ARCH)/ clean
.PHONY: clean

PHONY += FORCE
FORCE:

include scripts/build/common.mk

-include $(OBJS:.o=.d)
