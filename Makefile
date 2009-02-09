CLASSPATH_CONFIG = tools/classpath-config

JAMVM_INSTALL_DIR	:= /usr/local
CLASSPATH_INSTALL_DIR	?= $(shell ./tools/classpath-config)

GLIBJ		= $(CLASSPATH_INSTALL_DIR)/share/classpath/glibj.zip
BOOTCLASSPATH	= lib/classes.zip:$(GLIBJ)

JAMVM_ARCH	:= $(shell uname -m | sed -e s/i.86/i386/ | sed -e s/ppc/powerpc/)
ARCH		:= $(shell uname -m | sed -e s/i.86/i386/)
OS		:= $(shell uname -s | tr "[:upper:]" "[:lower:]")

ifeq ($(ARCH),i386)
ARCH		= x86
ARCH_POSTFIX	= _32
endif

ifeq ($(ARCH),x86_64)
ARCH		= x86
ARCH_POSTFIX	= _64
endif

ifeq ($(ARCH),ppc)
ARCH		= ppc
ARCH_POSTFIX	= _32
endif

ARCH_CONFIG=include/arch-$(ARCH)/config$(ARCH_POSTFIX).h

# Make the build silent by default
V =

BIN_DIR		:= bin
PROGRAM		:= $(BIN_DIR)/java

include arch/$(ARCH)/Makefile$(ARCH_POSTFIX)

JIT_OBJS = \
	jit/alloc.o		\
	jit/arithmetic-bc.o	\
	jit/basic-block.o	\
	jit/branch-bc.o		\
	jit/bytecode-to-ir.o	\
	jit/cfg-analyzer.o	\
	jit/compilation-unit.o	\
	jit/compiler.o		\
	jit/disass-common.o	\
	jit/disass.o		\
	jit/emit.o		\
	jit/emulate.o		\
	jit/expression.o	\
	jit/args.o		\
	jit/interval.o		\
	jit/invoke-bc.o		\
	jit/linear-scan.o	\
	jit/liveness.o		\
	jit/load-store-bc.o	\
	jit/exception-bc.o	\
	jit/object-bc.o		\
	jit/ostack-bc.o		\
	jit/stack-slot.o	\
	jit/statement.o		\
	jit/spill-reload.o	\
	jit/trace-jit.o		\
	jit/trampoline.o	\
	jit/tree-printer.o	\
	jit/typeconv-bc.o	\
	jit/vtable.o

VM_OBJS = \
	vm/bitset.o		\
	vm/buffer.o		\
	vm/bytecode.o		\
	vm/bytecodes.o		\
	vm/debug-dump.o		\
	vm/natives.o		\
	vm/resolve.o		\
	vm/stack.o		\
	vm/string.o		\
	vm/types.o		\
	vm/zalloc.o		\
	vm/class.o

JAMVM_OBJS = \
	jato/jato.o		\
	jamvm/access.o		\
	jamvm/alloc.o		\
	jamvm/cast.o		\
	jamvm/class.o		\
	jamvm/direct.o		\
	jamvm/dll.o		\
	jamvm/dll_ffi.o		\
	jamvm/excep.o		\
	jamvm/execute.o		\
	jamvm/frame.o		\
	jamvm/hash.o		\
	jamvm/hooks.o		\
	jamvm/init.o		\
	jamvm/interp.o		\
	jamvm/jni.o		\
	jamvm/lock.o		\
	jamvm/natives.o		\
	jamvm/properties.o	\
	jamvm/reflect.o		\
	jamvm/resolve.o		\
	jamvm/string.o		\
	jamvm/thread.o		\
	jamvm/utf8.o		\
	jamvm/zip.o

LIBHARNESS_OBJS = \
	test/libharness/libharness.o

JATO_OBJS = $(ARCH_OBJS) $(JIT_OBJS) $(VM_OBJS)

OBJS = $(JAMVM_OBJS) $(JATO_OBJS)

AS		:= as
CC		:= gcc
MONOBURG	:= ./monoburg/monoburg
JAVAC		:= ecj

CFLAGS		+= -g -Wall -rdynamic -std=gnu99

WARNINGS	= -Wsign-compare -Wundef
CFLAGS		+= $(WARNINGS)

OPTIMIZATIONS	+= -Os
CFLAGS		+= $(OPTIMIZATIONS)

INCLUDES	= -Iinclude -Ijit -Ijamvm -Ijit/glib -include $(ARCH_CONFIG) $(ARCH_INCLUDES)
CFLAGS		+= $(INCLUDES)

DEFINES = -DINSTALL_DIR=\"$(JAMVM_INSTALL_DIR)\" -DCLASSPATH_INSTALL_DIR=\"$(CLASSPATH_INSTALL_DIR)\"
CFLAGS		+= $(DEFINES)

LIBS		= -lpthread -lm -ldl -lz -lbfd -lopcodes -liberty $(ARCH_LIBS)

ARCH_INCLUDE_DIR = include/arch
JAMVM_ARCH_H = include/vm/arch.h

all: $(PROGRAM) test
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
	$(Q) $(CC) -c $(CFLAGS) $< -o $@

%.o: %.S
	$(E) "  AS      " $@
	$(Q) $(AS) $(AFLAGS) $< -o $@

$(JAMVM_ARCH_H):
	$(E) "  LN      " $@
	$(Q) ln -fsn ../../jamvm/arch/$(JAMVM_ARCH).h $@

arch/$(ARCH)/insn-selector.c: FORCE
	$(E) "  MONOBURG" $@
	$(Q) $(MONOBURG) -p -e arch/$(ARCH)/insn-selector.brg > $@

$(PROGRAM): lib monoburg $(ARCH_INCLUDE_DIR) $(BIN_DIR) $(JAMVM_ARCH_H) compile
	$(E) "  CC      " $@
	$(Q) $(CC) $(CFLAGS) $(OBJS) -o $(PROGRAM) $(LIBS)

compile: $(OBJS)

$(ARCH_INCLUDE_DIR): FORCE
	$(E) "  LN      " $@
	$(Q) ln -fsn arch-$(ARCH) $@

$(BIN_DIR): FORCE
	$(E) "  MKDIR   " $@
	$(Q) mkdir -p $@

test: $(ARCH_INCLUDE_DIR) $(JAMVM_ARCH_H) monoburg
	make -C test/vm/ ARCH=$(ARCH) test
	make -C test/jit/ ARCH=$(ARCH) test
	make -C test/arch-$(ARCH)/ test
.PHONY: test

%.class: %.java
	$(E) "  JAVAC   " $@
	$(Q) $(JAVAC) -cp $(BOOTCLASSPATH):regression -d regression $<

REGRESSION_TEST_SUITE_CLASSES = \
	regression/jamvm/TestCase.class \
	regression/jamvm/ExitStatusIsOneTest.class \
	regression/jamvm/ExitStatusIsZeroTest.class \
	regression/jamvm/LoadConstantsTest.class \
	regression/jamvm/IntegerArithmeticTest.class \
	regression/jamvm/LongArithmeticTest.class \
	regression/jamvm/ObjectCreationAndManipulationTest.class \
	regression/jamvm/ControlTransferTest.class \
	regression/jamvm/SynchronizationTest.class \
	regression/jamvm/MethodInvocationAndReturnTest.class \
	regression/jamvm/ConversionTest.class

lib: $(CLASSPATH_CONFIG)
	make -C lib/ JAVAC=$(JAVAC) GLIBJ=$(GLIBJ)
.PHONY: lib

regression: monoburg $(CLASSPATH_CONFIG) lib $(PROGRAM) $(REGRESSION_TEST_SUITE_CLASSES)
	$(E) "  REGRESSION"
	$(Q) cd regression && /bin/bash run-suite.sh $(JAVA_OPTS)
.PHONY: regression

clean:
	$(E) "  CLEAN"
	$(Q) - rm -rf $(BIN_DIR)
	$(Q) - rm -f $(CLASSPATH_CONFIG)
	$(Q) - rm -f $(OBJS)
	$(Q) - rm -f $(LIBHARNESS_OBJS)
	$(Q) - rm -f $(ARCH_TEST_OBJS)
	$(Q) - rm -f arch/$(ARCH)/insn-selector.c
	$(Q) - rm -f $(PROGRAM)
	$(Q) - rm -f $(JAMVM_ARCH_H)
	$(Q) - rm -f $(ARCH_TEST_SUITE)
	$(Q) - rm -f test-suite.o
	$(Q) - rm -f $(ARCH_TESTRUNNER)
	$(Q) - rm -f $(REGRESSION_TEST_SUITE_CLASSES)
	$(Q) - find regression/ -name "*.class" | xargs rm -f
	$(Q) - rm -f tags
	$(Q) - rm -f include/arch
	$(Q) - make -C monoburg/ clean
	$(Q) - make -C test/vm/ clean
	$(Q) - make -C test/jit/ clean
	$(Q) - make -C test/arch-$(ARCH)/ clean
	$(Q) - make -C lib clean
.PHONY: clean

PHONY += FORCE
FORCE:

include scripts/build/common.mk
