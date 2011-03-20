VERSION = 0.1.1

CLASSPATH_CONFIG = tools/classpath-config

CLASSPATH_INSTALL_DIR	?= $(shell ./tools/classpath-config)

GLIBJ		= $(CLASSPATH_INSTALL_DIR)/share/classpath/glibj.zip

MAKEFLAGS += --no-print-directory

uname_M		:= $(shell uname -m | sed -e s/i.86/i386/ | sed -e s/i86pc/i386/)
ARCH		:= $(shell sh scripts/gcc-arch.sh $(CC))
SYS		:= $(shell uname -s | tr A-Z a-z)

include scripts/build/arch.mk

ARCH_CONFIG=arch/$(ARCH)/include/arch/config$(ARCH_POSTFIX).h

# Make the build silent by default
V =

PROGRAM		:= jato

LIB_FILE	:= libjvm.a

include arch/$(ARCH)/Makefile$(ARCH_POSTFIX)
include sys/$(SYS)-$(ARCH)/Makefile

OBJS += vm/jato.o

LIB_OBJS += $(ARCH_OBJS)
LIB_OBJS += $(SYS_OBJS)

LIB_OBJS += cafebabe/annotations_attribute.o
LIB_OBJS += cafebabe/attribute_array.o
LIB_OBJS += cafebabe/attribute_info.o
LIB_OBJS += cafebabe/class.o
LIB_OBJS += cafebabe/code_attribute.o
LIB_OBJS += cafebabe/constant_pool.o
LIB_OBJS += cafebabe/constant_value_attribute.o
LIB_OBJS += cafebabe/enclosing_method_attribute.o
LIB_OBJS += cafebabe/error.o
LIB_OBJS += cafebabe/exceptions_attribute.o
LIB_OBJS += cafebabe/field_info.o
LIB_OBJS += cafebabe/inner_classes_attribute.o
LIB_OBJS += cafebabe/line_number_table_attribute.o
LIB_OBJS += cafebabe/method_info.o
LIB_OBJS += cafebabe/source_file_attribute.o
LIB_OBJS += cafebabe/stream.o
LIB_OBJS += jit/args.o
LIB_OBJS += jit/arithmetic-bc.o
LIB_OBJS += jit/basic-block.o
LIB_OBJS += jit/bc-offset-mapping.o
LIB_OBJS += jit/branch-bc.o
LIB_OBJS += jit/bytecode-to-ir.o
LIB_OBJS += jit/cfg-analyzer.o
LIB_OBJS += jit/compilation-unit.o
LIB_OBJS += jit/compiler.o
LIB_OBJS += jit/cu-mapping.o
LIB_OBJS += jit/dominance.o
LIB_OBJS += jit/elf.o
LIB_OBJS += jit/emit.o
LIB_OBJS += jit/emulate.o
LIB_OBJS += jit/exception-bc.o
LIB_OBJS += jit/exception.o
LIB_OBJS += jit/expression.o
LIB_OBJS += jit/fixup-site.o
LIB_OBJS += jit/gdb.o
LIB_OBJS += jit/interval.o
LIB_OBJS += jit/invoke-bc.o
LIB_OBJS += jit/linear-scan.o
LIB_OBJS += jit/liveness.o
LIB_OBJS += jit/load-store-bc.o
LIB_OBJS += jit/method.o
LIB_OBJS += jit/nop-bc.o
LIB_OBJS += jit/object-bc.o
LIB_OBJS += jit/ostack-bc.o
LIB_OBJS += jit/pc-map.o
LIB_OBJS += jit/perf-map.o
LIB_OBJS += jit/spill-reload.o
LIB_OBJS += jit/stack-slot.o
LIB_OBJS += jit/statement.o
LIB_OBJS += jit/subroutine.o
LIB_OBJS += jit/switch-bc.o
LIB_OBJS += jit/text.o
LIB_OBJS += jit/trace-jit.o
LIB_OBJS += jit/trampoline.o
LIB_OBJS += jit/tree-node.o
LIB_OBJS += jit/tree-printer.o
LIB_OBJS += jit/typeconv-bc.o
LIB_OBJS += jit/vtable.o
LIB_OBJS += jit/wide-bc.o
LIB_OBJS += lib/array.o
LIB_OBJS += lib/bitset.o
LIB_OBJS += lib/buffer.o
LIB_OBJS += lib/compile-lock.o
LIB_OBJS += lib/guard-page.o
LIB_OBJS += lib/hash-map.o
LIB_OBJS += lib/list.o
LIB_OBJS += lib/parse.o
LIB_OBJS += lib/pqueue.o
LIB_OBJS += lib/radix-tree.o
LIB_OBJS += lib/stack.o
LIB_OBJS += lib/string.o
LIB_OBJS += runtime/classloader.o
LIB_OBJS += runtime/java_lang_VMClass.o
LIB_OBJS += runtime/java_lang_VMRuntime.o
LIB_OBJS += runtime/java_lang_VMSystem.o
LIB_OBJS += runtime/java_lang_VMThread.o
LIB_OBJS += runtime/java_lang_reflect_VMMethod.o
LIB_OBJS += runtime/reflection.o
LIB_OBJS += runtime/stack-walker.o
LIB_OBJS += runtime/sun_misc_Unsafe.o
LIB_OBJS += vm/annotation.o
LIB_OBJS += vm/boehm-gc.o
LIB_OBJS += vm/bytecode.o
LIB_OBJS += vm/call.o
LIB_OBJS += vm/class.o
LIB_OBJS += vm/classloader.o
LIB_OBJS += vm/debug-dump.o
LIB_OBJS += vm/die.o
LIB_OBJS += vm/fault-inject.o
LIB_OBJS += vm/field.o
LIB_OBJS += vm/gc.o
LIB_OBJS += vm/itable.o
LIB_OBJS += vm/jar.o
LIB_OBJS += vm/jni-interface.o
LIB_OBJS += vm/jni.o
LIB_OBJS += vm/method.o
LIB_OBJS += vm/monitor.o
LIB_OBJS += vm/natives.o
LIB_OBJS += vm/object.o
LIB_OBJS += vm/preload.o
LIB_OBJS += vm/reference.o
LIB_OBJS += vm/signal.o
LIB_OBJS += vm/stack-trace.o
LIB_OBJS += vm/static.o
LIB_OBJS += vm/string.o
LIB_OBJS += vm/thread.o
LIB_OBJS += vm/trace.o
LIB_OBJS += vm/types.o
LIB_OBJS += vm/utf8.o
LIB_OBJS += vm/zalloc.o

RUNTIME_CLASSES =

CC		?= gcc
LINK		?= $(CC)
MONOBURG	:= ./tools/monoburg/monoburg
JAVA		?= $(shell pwd)/jato
JAVAC_OPTS	?= -encoding utf-8
INSTALL		?= install

ifeq ($(uname_M),x86_64)
JASMIN		?= java -jar tools/jasmin/jasmin.jar
JAVAC		?= JAVA=java ./tools/ecj
else
JASMIN		?= $(JAVA) -jar tools/jasmin/jasmin.jar
JAVAC		?= JAVA=$(JAVA) ./tools/ecj
endif

DEFAULT_CFLAGS	+= $(ARCH_CFLAGS) -g -rdynamic -std=gnu99 -D_GNU_SOURCE -fstack-protector-all -D_FORTIFY_SOURCE=2

# boehmgc integration (see boehmgc/doc/README.linux)
DEFAULT_CFLAGS  += -D_REENTRANT -DGC_LINUX_THREADS -DGC_USE_LD_WRAP

JATO_CFLAGS  += -Wl,--wrap -Wl,pthread_create -Wl,--wrap -Wl,pthread_join \
	     -Wl,--wrap -Wl,pthread_detach -Wl,--wrap -Wl,pthread_sigmask \
             -Wl,--wrap -Wl,sleep

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

INCLUDES	= -Iinclude -Iarch/$(ARCH)/include -Isys/$(SYS)-$(ARCH)/include -Ijit -Ijit/glib -include $(ARCH_CONFIG) -Iboehmgc/include
DEFAULT_CFLAGS	+= $(INCLUDES)

DEFAULT_LIBS	= -L. -ljvm -lrt -lpthread -lm -ldl -lz -lzip -lbfd -lopcodes -liberty -Lboehmgc -lboehmgc $(ARCH_LIBS)

all: $(PROGRAM)
.PHONY: all
.DEFAULT: all

VERSION_HEADER = include/vm/version.h

$(VERSION_HEADER): FORCE
	$(E) "  GEN     " $@
	$(Q) echo "#define JATO_VERSION \"$(VERSION)\"" > $(VERSION_HEADER)

$(CLASSPATH_CONFIG):
	$(E) "  LINK    " $@
	$(Q) $(LINK) -Wall $(CLASSPATH_CONFIG).c -o $(CLASSPATH_CONFIG)

monoburg:
	+$(Q) $(MAKE) -C tools/monoburg/
.PHONY: monoburg

boehmgc:
	+$(Q) ARCH=$(ARCH) $(MAKE) -C boehmgc/
.PHONY: boehmgc

%.o: %.c
	$(E) "  CC      " $@
	$(Q) $(CC) -MD -c $(DEFAULT_CFLAGS) $(CFLAGS) $< -o $@

# -gstabs is needed for correct backtraces.
%.o: %.S
	$(E) "  AS      " $@
	$(Q) $(CC) -c -gstabs $(DEFAULT_CFLAGS) $(CFLAGS) $< -o $@

arch/$(ARCH)/insn-selector$(ARCH_POSTFIX).c: monoburg FORCE
	$(E) "  MONOBURG" $@
	$(Q) $(MONOBURG) -p -e $(MB_DEFINES) $(@:.c=.brg) > $@

$(PROGRAM): $(LIB_FILE) $(OBJS) $(RUNTIME_CLASSES)
	$(E) "  LINK    " $@
	$(Q) $(LINK) $(JATO_CFLAGS) $(DEFAULT_CFLAGS) $(CFLAGS) $(OBJS) -o $(PROGRAM) $(LIBS) $(DEFAULT_LIBS)

$(LIB_FILE): monoburg boehmgc $(VERSION_HEADER) $(CLASSPATH_CONFIG) $(LIB_OBJS)
	$(E) "  AR      " $@
	$(Q) rm -f $@ && $(AR) rcs $@ $(LIB_OBJS)

check-unit: monoburg
	+$(MAKE) -C test/unit/vm/ SYS=$(SYS) ARCH=$(ARCH) ARCH_POSTFIX=$(ARCH_POSTFIX) $(TEST)
	+$(MAKE) -C test/unit/jit/ SYS=$(SYS) ARCH=$(ARCH) ARCH_POSTFIX=$(ARCH_POSTFIX) $(TEST)
	+$(MAKE) -C test/unit/arch-$(ARCH)/ SYS=$(SYS) ARCH=$(ARCH) ARCH_POSTFIX=$(ARCH_POSTFIX) $(TEST)
.PHONY: check-unit

check-integration: $(LIB_FILE)
	+$(MAKE) -C test/integration check
.PHONY: check-integration

REGRESSION_TEST_SUITE_CLASSES = \
	test/functional/jato/internal/VM.java \
	test/functional/java/lang/JNITest.java \
	test/functional/java/lang/reflect/ClassTest.java \
	test/functional/java/lang/reflect/MethodTest.java \
	test/functional/jvm/ArgsTest.java \
	test/functional/jvm/ArrayExceptionsTest.java \
	test/functional/jvm/ArrayMemberTest.java \
	test/functional/jvm/ArrayTest.java \
	test/functional/jvm/BranchTest.java \
	test/functional/jvm/CFGCrashTest.java \
	test/functional/jvm/ClassExceptionsTest.java \
	test/functional/jvm/ClassLoaderTest.java \
	test/functional/jvm/ClinitFloatTest.java \
	test/functional/jvm/CloneTest.java \
	test/functional/jvm/ControlTransferTest.java \
	test/functional/jvm/ConversionTest.java \
	test/functional/jvm/DoubleArithmeticTest.java \
	test/functional/jvm/DoubleConversionTest.java \
	test/functional/jvm/ExceptionsTest.java \
	test/functional/jvm/ExitStatusIsOneTest.java \
	test/functional/jvm/ExitStatusIsZeroTest.java \
	test/functional/jvm/FibonacciTest.java \
	test/functional/jvm/FinallyTest.java \
	test/functional/jvm/FloatArithmeticTest.java \
	test/functional/jvm/FloatConversionTest.java \
	test/functional/jvm/GcTortureTest.java \
	test/functional/jvm/GetstaticPatchingTest.java \
	test/functional/jvm/IntegerArithmeticExceptionsTest.java \
	test/functional/jvm/IntegerArithmeticTest.java \
	test/functional/jvm/InterfaceFieldInheritanceTest.java \
	test/functional/jvm/InterfaceInheritanceTest.java \
	test/functional/jvm/InvokeinterfaceTest.java \
	test/functional/jvm/InvokestaticPatchingTest.java \
	test/functional/jvm/LoadConstantsTest.java \
	test/functional/jvm/LongArithmeticExceptionsTest.java \
	test/functional/jvm/LongArithmeticTest.java \
	test/functional/jvm/MethodInvocationAndReturnTest.java \
	test/functional/jvm/MethodInvocationExceptionsTest.java \
	test/functional/jvm/MethodInvokeVirtualTest.java \
	test/functional/jvm/MonitorTest.java \
	test/functional/jvm/MultithreadingTest.java \
	test/functional/jvm/ObjectArrayTest.java \
	test/functional/jvm/ObjectCreationAndManipulationExceptionsTest.java \
	test/functional/jvm/ObjectCreationAndManipulationTest.java \
	test/functional/jvm/ObjectStackTest.java \
	test/functional/jvm/ParameterPassingTest.java \
	test/functional/jvm/PrintTest.java \
	test/functional/jvm/PutfieldTest.java \
	test/functional/jvm/PutstaticPatchingTest.java \
	test/functional/jvm/PutstaticTest.java \
	test/functional/jvm/RegisterAllocatorTortureTest.java \
	test/functional/jvm/StackTraceTest.java \
	test/functional/jvm/StringTest.java \
	test/functional/jvm/SwitchTest.java \
	test/functional/jvm/SynchronizationExceptionsTest.java \
	test/functional/jvm/SynchronizationTest.java \
	test/functional/jvm/TestCase.java \
	test/functional/jvm/TrampolineBackpatchingTest.java \
	test/functional/jvm/VirtualAbstractInterfaceMethodTest.java \
	test/functional/jvm/lang/reflect/FieldTest.java \
	test/functional/sun/misc/UnsafeTest.java \
	test/functional/test/java/lang/ClassTest.java

JASMIN_REGRESSION_TEST_SUITE_CLASSES = \
	test/functional/jvm/DupTest.j \
	test/functional/jvm/EntryTest.j \
	test/functional/jvm/ExceptionHandlerTest.j \
	test/functional/jvm/InvokeResultTest.j \
	test/functional/jvm/InvokeTest.j \
	test/functional/jvm/NoSuchMethodErrorTest.j\
	test/functional/jvm/PopTest.j \
	test/functional/jvm/SubroutineTest.j \
	test/functional/jvm/WideTest.j

compile-java-tests: $(PROGRAM) FORCE
	$(E) "  JAVAC   " $(REGRESSION_TEST_SUITE_CLASSES)
	$(Q) JAVA=$(JAVA) $(JAVAC) -source 1.5 -cp $(GLIBJ):test/functional $(JAVAC_OPTS) -d test/functional $(REGRESSION_TEST_SUITE_CLASSES)
.PHONY: compile-java-tests

compile-jasmin-tests: $(PROGRAM) FORCE
	$(E) "  JASMIN  " $(JASMIN_REGRESSION_TEST_SUITE_CLASSES)
	$(Q) $(JASMIN) $(JASMIN_OPTS) -d test/functional $(JASMIN_REGRESSION_TEST_SUITE_CLASSES) > /dev/null
.PHONY: compile-jasmin-tests

$(RUNTIME_CLASSES): %.class: %.java
	$(E) "  JAVAC   " $@
	$(Q) $(JAVAC) -cp $(GLIBJ) $(JAVAC_OPTS) -d runtime/classpath $<

lib: $(CLASSPATH_CONFIG)
	+$(MAKE) -C lib/ JAVAC=$(JAVAC) GLIBJ=$(GLIBJ)
.PHONY: lib

compile-jni-test-lib:
	+$(MAKE) -C test/functional/jni CC='$(CC)'
.PHONY: compile-jni-test-lib

check-functional: monoburg $(CLASSPATH_CONFIG) $(PROGRAM) compile-java-tests compile-jasmin-tests compile-jni-test-lib
	$(E) "  REGRESSION"
	$(Q) ./tools/test.py
.PHONY: check-functional

check: check-unit check-integration check-functional
.PHONY: check

torture:
	$(E) "  MAKE"
	$(Q) $(MAKE) -C torture JAVA=$(JAVA)
.PHONY: torture

clean:
	$(E) "  CLEAN"
	$(Q) - rm -f $(PROGRAM)
	$(Q) - rm -f $(LIB_FILE)
	$(Q) - rm -f $(VERSION_HEADER)
	$(Q) - rm -f $(CLASSPATH_CONFIG)
	$(Q) - rm -f $(OBJS)
	$(Q) - rm -f $(OBJS:.o=.d)
	$(Q) - rm -f $(LIB_OBJS)
	$(Q) - rm -f $(LIB_OBJS:.o=.d)
	$(Q) - rm -f $(ARCH_TEST_OBJS)
	$(Q) - rm -f arch/$(ARCH)/insn-selector.c
	$(Q) - rm -f $(PROGRAM)
	$(Q) - rm -f $(ARCH_TEST_SUITE)
	$(Q) - rm -f test-suite.o
	$(Q) - rm -f $(ARCH_TESTRUNNER)
	$(Q) - rm -f $(RUNTIME_CLASSES)
	$(Q) - find test/functional/ -name "*.class" | xargs rm -f
	$(Q) - find runtime/ -name "*.class" | xargs rm -f
	$(Q) - rm -f tags
	$(Q) - rm -f include/arch
	+$(Q) - $(MAKE) -C tools/monoburg/ clean >/dev/null
	+$(Q) - $(MAKE) -C boehmgc/ clean >/dev/null
	+$(Q) - $(MAKE) -C test/functional/jni/ clean >/dev/null
	+$(Q) - $(MAKE) -C test/unit/vm/ clean >/dev/null
	+$(Q) - $(MAKE) -C test/unit/jit/ clean >/dev/null
	+$(Q) - $(MAKE) -C test/unit/arch-$(ARCH)/ clean >/dev/null
	+$(Q) - $(MAKE) -C test/integration/ clean >/dev/null
.PHONY: clean

INSTALL_PREFIX	?= $(HOME)

install: $(PROGRAM)
	$(E) "  INSTALL " $(PROGRAM)
	$(Q) $(INSTALL) -d -m 755 $(INSTALL_PREFIX)/bin
	$(Q) $(INSTALL) $(PROGRAM) $(INSTALL_PREFIX)/bin
.PHONY: install

tags: FORCE
	$(E) "  TAGS"
	$(Q) rm -f tags
	$(Q) ctags-exuberant -a -R arch/
	$(Q) ctags-exuberant -a -R include
	$(Q) ctags-exuberant -a -R jit/
	$(Q) ctags-exuberant -a -R lib/
	$(Q) ctags-exuberant -a -R runtime/
	$(Q) ctags-exuberant -a -R cafebabe/
	$(Q) ctags-exuberant -a -R vm/

PHONY += FORCE
FORCE:

include scripts/build/common.mk

-include $(OBJS:.o=.d)
