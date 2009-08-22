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
WARNINGS	+= -Werror
MB_DEFINES	+= -DCONFIG_X86_32
ifeq ($(BUILD_ARCH),x86_64)
ARCH_CFLAGS	+= -m32
TEST		= test
endif
endif

ifeq ($(ARCH),x86_64)
override ARCH	= x86
ARCH_POSTFIX	= _64
ARCH_CFLAGS	+= -fno-omit-frame-pointer
MB_DEFINES	+= -DCONFIG_X86_64
endif

ifeq ($(ARCH),ppc)
override ARCH	= ppc
ARCH_POSTFIX	= _32
MB_DEFINES	+= -DCONFIG_PPC
endif

export ARCH_CFLAGS

ARCH_CONFIG=arch/$(ARCH)/include/arch/config$(ARCH_POSTFIX).h

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
	jit/gc-map.o		\
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
	jit/switch-bc.o		\
	jit/text.o		\
	jit/trace-jit.o		\
	jit/trampoline.o	\
	jit/tree-node.o		\
	jit/tree-printer.o	\
	jit/typeconv-bc.o	\
	jit/vtable.o		\
	jit/subroutine.o	\
	jit/pc-map.o		\
	jit/wide-bc.o

VM_OBJS = \
	vm/bytecode.o		\
	vm/bytecodes.o		\
	vm/call.o		\
	vm/class.o		\
	vm/classloader.o	\
	vm/debug-dump.o		\
	vm/die.o		\
	vm/fault-inject.o 	\
	vm/field.o		\
	vm/gc.o			\
	vm/itable.o		\
	vm/jar.o		\
	vm/jato.o		\
	vm/jni-interface.o	\
	vm/jni.o		\
	vm/method.o		\
	vm/natives.o		\
	vm/object.o		\
	vm/preload.o		\
	vm/reflection.o		\
	vm/resolve.o		\
	vm/signal.o		\
	vm/stack-trace.o	\
	vm/static.o		\
	vm/string.o		\
	vm/thread.o		\
	vm/trace.o		\
	vm/types.o		\
	vm/unsafe.o		\
	vm/utf8.o		\
	vm/zalloc.o

LIB_OBJS = \
	lib/bitset.o		\
	lib/buffer.o		\
	lib/guard-page.o	\
	lib/hash-map.o		\
	lib/list.o		\
	lib/pqueue.o		\
	lib/radix-tree.o	\
	lib/stack.o		\
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
	line_number_table_attribute.o	\
	method_info.o			\
	source_file_attribute.o		\
	stream.o

CAFEBABE_OBJS := $(addprefix cafebabe/src/cafebabe/,$(CAFEBABE_OBJS))

LIBHARNESS_OBJS = \
	test/libharness/libharness.o

JATO_OBJS = $(ARCH_OBJS) $(JIT_OBJS) $(VM_OBJS) $(LIB_OBJS) $(CAFEBABE_OBJS)

OBJS = $(JAMVM_OBJS) $(JATO_OBJS)

RUNTIME_CLASSES =

CC		?= gcc
LINK		?= $(CC)
MONOBURG	:= ./tools/monoburg/monoburg
JAVAC		?= ecj
JASMIN		:= java -jar tools/jasmin/jasmin.jar
JAVAC_OPTS	:= -encoding utf-8
INSTALL		:= install

DEFAULT_CFLAGS	+= $(ARCH_CFLAGS) -g -rdynamic -std=gnu99 -D_GNU_SOURCE -fstack-protector-all -D_FORTIFY_SOURCE=2

# XXX: Temporary hack -Vegard
DEFAULT_CFLAGS	+= -DNOT_IMPLEMENTED='fprintf(stderr, "%s:%d: warning: %s not implemented\n", __FILE__, __LINE__, __func__)'

WARNINGS	+=				\
		-Wall				\
		-Wcast-align			\
		-Wformat=2			\
		-Winit-self			\
		-Wmissing-declarations		\
		-Wmissing-prototypes		\
		-Wnested-externs		\
		-Wno-system-headers		\
		-Wold-style-definition		\
		-Wredundant-decls		\
		-Wsign-compare			\
		-Wstrict-prototypes		\
		-Wundef				\
		-Wvolatile-register-var		\
		-Wwrite-strings

DEFAULT_CFLAGS	+= $(WARNINGS)

OPTIMIZATIONS	+= -Os -fno-delete-null-pointer-checks
DEFAULT_CFLAGS	+= $(OPTIMIZATIONS)

INCLUDES	= -Iinclude -Iarch/$(ARCH)/include -Ijit -Ijit/glib -Icafebabe/include -include $(ARCH_CONFIG)
DEFAULT_CFLAGS	+= $(INCLUDES)

DEFINES = -DINSTALL_DIR=\"$(JAMVM_INSTALL_DIR)\" -DCLASSPATH_INSTALL_DIR=\"$(CLASSPATH_INSTALL_DIR)\"
DEFAULT_CFLAGS	+= $(DEFINES)

DEFAULT_LIBS	= -lrt -lpthread -lm -ldl -lz -lzip -lbfd -lopcodes -liberty $(ARCH_LIBS)

all: $(PROGRAM) $(TEST)
.PHONY: all
.DEFAULT: all

$(CLASSPATH_CONFIG):
	$(E) "  LINK    " $@
	$(Q) $(LINK) -Wall $(CLASSPATH_CONFIG).c -o $(CLASSPATH_CONFIG)

monoburg:
	$(Q) make -C tools/monoburg/
.PHONY: monoburg

%.o: %.c
	$(E) "  CC      " $@
	$(Q) $(CC) -c $(DEFAULT_CFLAGS) $(CFLAGS) $< -o $@
	$(Q) $(CC) -MM $(DEFAULT_CFLAGS) $(CFLAGS) -MT $@ $*.c -o $*.d

%.o: %.S
	$(E) "  AS      " $@
	$(Q) $(CC) -c $(DEFAULT_CFLAGS) $(CFLAGS) $< -o $@

arch/$(ARCH)/insn-selector.c: monoburg FORCE
	$(E) "  MONOBURG" $@
	$(Q) $(MONOBURG) -p -e $(MB_DEFINES) $(@:.c=.brg) > $@

$(PROGRAM): monoburg $(CLASSPATH_CONFIG) compile $(RUNTIME_CLASSES)
	$(E) "  LINK    " $@
	$(Q) $(LINK) $(DEFAULT_CFLAGS) $(CFLAGS) $(OBJS) -o $(PROGRAM) $(LIBS) $(DEFAULT_LIBS)

compile: $(OBJS)

test: monoburg
	make -C test/vm/ ARCH=$(ARCH) ARCH_POSTFIX=$(ARCH_POSTFIX) $(TEST)
	make -C test/jit/ ARCH=$(ARCH) ARCH_POSTFIX=$(ARCH_POSTFIX) $(TEST)
	make -C test/arch-$(ARCH)/ ARCH=$(ARCH) ARCH_POSTFIX=$(ARCH_POSTFIX) $(TEST)
.PHONY: test

REGRESSION_TEST_SUITE_CLASSES = \
	regression/jato/internal/VM.java \
	regression/java/lang/VMClassTest.java \
	regression/jvm/ArgsTest.java \
	regression/jvm/ArrayExceptionsTest.java \
	regression/jvm/ArrayMemberTest.java \
	regression/jvm/ArrayTest.java \
	regression/jvm/BranchTest.java \
	regression/jvm/CFGCrashTest.java \
	regression/jvm/ClassExceptionsTest.java \
	regression/jvm/ClassLoaderTest.java \
	regression/jvm/ClassTest.java \
	regression/jvm/CloneTest.java \
	regression/jvm/ControlTransferTest.java \
	regression/jvm/ConversionTest.java \
	regression/jvm/DoubleArithmeticTest.java \
	regression/jvm/DoubleConversionTest.java \
	regression/jvm/ExceptionsTest.java \
	regression/jvm/ExitStatusIsOneTest.java \
	regression/jvm/ExitStatusIsZeroTest.java \
	regression/jvm/FibonacciTest.java \
	regression/jvm/FinallyTest.java \
	regression/jvm/FloatArithmeticTest.java \
	regression/jvm/FloatConversionTest.java \
	regression/jvm/GetstaticPatchingTest.java \
	regression/jvm/IntegerArithmeticExceptionsTest.java \
	regression/jvm/IntegerArithmeticTest.java \
	regression/jvm/InterfaceFieldInheritanceTest.java \
	regression/jvm/InterfaceInheritanceTest.java \
	regression/jvm/InvokeinterfaceTest.java \
	regression/jvm/InvokestaticPatchingTest.java \
	regression/jvm/LoadConstantsTest.java \
	regression/jvm/LongArithmeticExceptionsTest.java \
	regression/jvm/LongArithmeticTest.java \
	regression/jvm/MethodInvocationAndReturnTest.java \
	regression/jvm/MethodInvocationExceptionsTest.java \
	regression/jvm/MultithreadingTest.java \
	regression/jvm/ObjectArrayTest.java \
	regression/jvm/ObjectCreationAndManipulationExceptionsTest.java \
	regression/jvm/ObjectCreationAndManipulationTest.java \
	regression/jvm/ObjectStackTest.java \
	regression/jvm/ParameterPassingTest.java \
	regression/jvm/PrintTest.java \
	regression/jvm/PutfieldTest.java \
	regression/jvm/PutstaticPatchingTest.java \
	regression/jvm/PutstaticTest.java \
	regression/jvm/RegisterAllocatorTortureTest.java \
	regression/jvm/StackTraceTest.java \
	regression/jvm/StringTest.java \
	regression/jvm/SwitchTest.java \
	regression/jvm/SynchronizationExceptionsTest.java \
	regression/jvm/SynchronizationTest.java \
	regression/jvm/TestCase.java \
	regression/jvm/TrampolineBackpatchingTest.java \
	regression/sun/misc/UnsafeTest.java

JASMIN_REGRESSION_TEST_SUITE_CLASSES = \
	regression/jvm/DupTest.j \
	regression/jvm/PopTest.j \
	regression/jvm/SubroutineTest.j \
	regression/jvm/WideTest.j \
	regression/jvm/InvokeResultTest.j

java-regression: FORCE
	$(E) "  JAVAC   " $(REGRESSION_TEST_SUITE_CLASSES)
	$(Q) $(JAVAC) -cp $(GLIBJ):regression $(JAVAC_OPTS) -d regression $(REGRESSION_TEST_SUITE_CLASSES)
.PHONY: java-regression

jasmin-regression: FORCE
	$(E) "  JASMIN  " $(JASMIN_REGRESSION_TEST_SUITE_CLASSES)
	$(Q) $(JASMIN) $(JASMIN_OPTS) -d regression $(JASMIN_REGRESSION_TEST_SUITE_CLASSES) > /dev/null
.PHONY: jasmin-regression

$(RUNTIME_CLASSES): %.class: %.java
	$(E) "  JAVAC   " $@
	$(Q) $(JAVAC) -cp $(GLIBJ) $(JAVAC_OPTS) -d runtime/classpath $<

lib: $(CLASSPATH_CONFIG)
	make -C lib/ JAVAC=$(JAVAC) GLIBJ=$(GLIBJ)
.PHONY: lib

regression: monoburg $(CLASSPATH_CONFIG) $(PROGRAM) java-regression jasmin-regression
	$(E) "  REGRESSION"
	$(Q) cd regression && /bin/bash run-suite.sh $(JAVA_OPTS)
.PHONY: regression

check: test regression
.PHONY: check

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

INSTALL_PREFIX	?= $(HOME)

install: $(PROGRAM)
	$(E) "  INSTALL " $(PROGRAM)
	$(Q) $(INSTALL) -d -m 755 $(INSTALL_PREFIX)/bin
	$(Q) $(INSTALL) $(PROGRAM) $(INSTALL_PREFIX)/bin
.PHONY: install

PHONY += FORCE
FORCE:

include scripts/build/common.mk

-include $(OBJS:.o=.d)
