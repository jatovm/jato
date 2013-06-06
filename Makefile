VERSION = 0.3

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

PROGRAMS = jato

LIB_FILE	:= libjvm.a

include arch/$(ARCH)/Makefile$(ARCH_POSTFIX)
include sys/$(SYS)-$(ARCH)/Makefile

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
LIB_OBJS += cafebabe/stack_map_table_attribute.o
LIB_OBJS += cafebabe/stream.o
LIB_OBJS += jit/abc-removal.o
LIB_OBJS += jit/args.o
LIB_OBJS += jit/arithmetic-bc.o
LIB_OBJS += jit/basic-block.o
LIB_OBJS += jit/bc-offset-mapping.o
LIB_OBJS += jit/branch-bc.o
LIB_OBJS += jit/bytecode-to-ir.o
LIB_OBJS += jit/cfg-analyzer.o
LIB_OBJS += jit/clobber.o
LIB_OBJS += jit/compilation-unit.o
LIB_OBJS += jit/compiler.o
LIB_OBJS += jit/constant-pool.o
LIB_OBJS += jit/cu-mapping.o
LIB_OBJS += jit/dce.o
LIB_OBJS += jit/dominance.o
LIB_OBJS += jit/elf.o
LIB_OBJS += jit/emit.o
LIB_OBJS += jit/emulate.o
LIB_OBJS += jit/exception-bc.o
LIB_OBJS += jit/exception.o
LIB_OBJS += jit/expression.o
LIB_OBJS += jit/fixup-site.o
LIB_OBJS += jit/gdb.o
LIB_OBJS += jit/inline-cache.o
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
LIB_OBJS += jit/ssa.o
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
LIB_OBJS += lib/arena.o
LIB_OBJS += lib/array.o
LIB_OBJS += lib/bitset.o
LIB_OBJS += lib/buffer.o
LIB_OBJS += lib/compile-lock.o
LIB_OBJS += lib/guard-page.o
LIB_OBJS += lib/hash-map.o
LIB_OBJS += lib/list.o
LIB_OBJS += lib/options.o
LIB_OBJS += lib/parse.o
LIB_OBJS += lib/pqueue.o
LIB_OBJS += lib/radix-tree.o
LIB_OBJS += lib/stack.o
LIB_OBJS += lib/symbol.o
LIB_OBJS += lib/string.o
LIB_OBJS += lib/zip.o
LIB_OBJS += runtime/gnu_java_lang_management_VMThreadMXBeanImpl.o
LIB_OBJS += runtime/java_lang_VMClass.o
LIB_OBJS += runtime/java_lang_VMClassLoader.o
LIB_OBJS += runtime/java_lang_VMRuntime.o
LIB_OBJS += runtime/java_lang_VMString.o
LIB_OBJS += runtime/java_lang_VMSystem.o
LIB_OBJS += runtime/java_lang_VMThread.o
LIB_OBJS += runtime/java_lang_reflect_VMField.o
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
LIB_OBJS += vm/debug.o
LIB_OBJS += vm/die.o
LIB_OBJS += vm/fault-inject.o
LIB_OBJS += vm/field.o
LIB_OBJS += vm/gc.o
LIB_OBJS += vm/interp.o
LIB_OBJS += vm/itable.o
LIB_OBJS += vm/jar.o
LIB_OBJS += vm/jni-interface.o
LIB_OBJS += vm/jni.o
LIB_OBJS += vm/method.o
LIB_OBJS += vm/verify-functions.o
LIB_OBJS += vm/verifier.o
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

CC		?= gcc
LINK		?= $(CC)
MONOBURG	:= ./tools/monoburg/monoburg
JAVA		?= $(shell pwd)/jato
JAVAC		?= ecj
INSTALL		?= install

ifeq ($(uname_M),x86_64)
JASMIN		?= java -jar tools/jasmin/jasmin.jar
else
JASMIN		?= $(JAVA) -jar tools/jasmin/jasmin.jar
endif

JAVAC_OPTS	+= -encoding utf-8
JAVAC_OPTS	+= -source 1.6 -target 1.6

DEFAULT_CFLAGS	+= $(ARCH_CFLAGS) -g -rdynamic -std=gnu99 -D_GNU_SOURCE

ifeq ($(uname_M),x86_64)
STACK_PROTECTOR = -fno-stack-protector
else
STACK_PROTECTOR = -fstack-protector-all -D_FORTIFY_SOURCE=2
endif

DEFAULT_CFLAGS += $(STACK_PROTECTOR)

HAS_TCMALLOC_MINIMAL:=$(shell scripts/gcc-has-lib.sh gcc tcmalloc_minimal)
ifeq ($(HAS_TCMALLOC_MINIMAL),y)
	TCMALLOC_CFLAGS += -ltcmalloc_minimal -fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free
endif

HAS_TCMALLOC:=$(shell scripts/gcc-has-lib.sh gcc tcmalloc)
ifeq ($(HAS_TCMALLOC),y)
	TCMALLOC_CFLAGS += -ltcmalloc -fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free
endif

HAS_LIBBFD:=$(shell scripts/gcc-has-lib.sh gcc bfd)
ifneq ($(HAS_LIBBFD),y)
    $(error No libbfd found, please install binutils-devel or binutils-dev package.)
endif

HAS_LIBFFI:=$(shell scripts/gcc-has-lib.sh gcc ffi)
ifneq ($(HAS_LIBFFI),y)
    $(error No libffi found, please install libffi-devel or libffi-dev package.)
endif

DEFAULT_CFLAGS	+= $(TCMALLOC_CFLAGS)
# boehmgc integration (see boehmgc/doc/README.linux)
DEFAULT_CFLAGS  += -D_REENTRANT -DGC_LINUX_THREADS -DGC_USE_LD_WRAP

JATO_CFLAGS  += -Wl,--wrap -Wl,pthread_create -Wl,--wrap -Wl,pthread_join \
	     -Wl,--wrap -Wl,pthread_detach -Wl,--wrap -Wl,pthread_sigmask \
             -Wl,--wrap -Wl,sleep

# XXX: Temporary hack -Vegard
DEFAULT_CFLAGS	+= -DNOT_IMPLEMENTED='fprintf(stderr, "%s:%d: warning: %s not implemented\n", __FILE__, __LINE__, __func__)'

EXTRA_WARNINGS =
EXTRA_WARNINGS += -Wformat
EXTRA_WARNINGS += -Wformat-security
EXTRA_WARNINGS += -Wformat-y2k
EXTRA_WARNINGS += -Winit-self
EXTRA_WARNINGS += -Wmissing-declarations
EXTRA_WARNINGS += -Wmissing-prototypes
EXTRA_WARNINGS += -Wnested-externs
EXTRA_WARNINGS += -Wno-system-headers
EXTRA_WARNINGS += -Wno-unused-parameter
EXTRA_WARNINGS += -Wold-style-definition
EXTRA_WARNINGS += -Wredundant-decls
EXTRA_WARNINGS += -Wstrict-prototypes
EXTRA_WARNINGS += -Wswitch-default
EXTRA_WARNINGS += -Wundef
EXTRA_WARNINGS += -Wwrite-strings
EXTRA_WARNINGS += -Wcast-align

DEFAULT_CFLAGS += -Wall -Wextra $(EXTRA_WARNINGS)

ifeq ($(uname_M),i386)
OPTIMIZATION_LEVEL = -O3
else
OPTIMIZATION_LEVEL = -Os
endif

OPTIMIZATIONS	+= $(OPTIMIZATION_LEVEL)
OPTIMIZATIONS	+= -fno-delete-null-pointer-checks
OPTIMIZATIONS	+= -fno-omit-frame-pointer

DEFAULT_CFLAGS	+= $(OPTIMIZATIONS)

INCLUDES	= -Iarch/$(ARCH)/include -Iinclude -Isys/$(SYS)-$(ARCH)/include -Ijit -Ijit/glib -include $(ARCH_CONFIG) -Iboehmgc/include
DEFAULT_CFLAGS	+= $(INCLUDES)

# Disable optimizations and turn on extra debugging information, if needed.
DEBUG =
ifeq ($(strip $(DEBUG)),1)
	# This overrides DEFAULT_CFLAGS, but not user-supplied CFLAGS.
	DEFAULT_CFLAGS += -O0 -ggdb -DCONFIG_DEBUG
endif

DEFAULT_LIBS	= -L. -ljvm -lrt -lpthread -lm -ldl -lz -lbfd -lopcodes -liberty -Lboehmgc -lboehmgc $(ARCH_LIBS)

all: $(PROGRAMS)
.PHONY: all
.DEFAULT: all

VERSION_HEADER = include/vm/version.h

ASM_OFFSETS_HEADER = arch/$(ARCH)/include/arch/asm-offsets.h
GEN_ASM_OFFSETS_SRC = arch/$(ARCH)/asm-offsets.c
GEN_ASM_OFFSETS = arch/$(ARCH)/asm-offsets

$(VERSION_HEADER): FORCE
	$(E) "  GEN     " $@
	$(Q) echo "#define JATO_VERSION \"$(VERSION)\"" > $(VERSION_HEADER)

$(ASM_OFFSETS_HEADER): FORCE
	$(E) "  GEN     " $@
	$(Q) echo "#ifndef ASM_OFFSETS_H" > $@
	$(Q) echo "#define ASM_OFFSETS_H" >> $@
	$(Q) echo "/*" >> $@
	$(Q) echo " * Autogenerated file: DO NOT EDIT" >> $@
	$(Q) echo "*/" >> $@
	$(Q) -test -f $(GEN_ASM_OFFSETS_SRC) && \
	     $(CC) -MD $(DEFAULT_CFLAGS) $(CFLAGS) $(GEN_ASM_OFFSETS_SRC) -o $(GEN_ASM_OFFSETS) && \
	     $(GEN_ASM_OFFSETS) >> $@
	$(Q) echo "#endif /* ASM_OFFSETS_H */" >> $@

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


jato.o: $(VERSION_HEADER)

$(foreach p,$(PROGRAMS),$(eval $(p): $(LIB_FILE)))
$(PROGRAMS): % : %.o
	$(E) "  LINK    " $@
	$(Q) $(LINK) $(JATO_CFLAGS) -o $@ $^ $(DEFAULT_LIBS)

$(LIB_FILE): monoburg boehmgc $(VERSION_HEADER) $(ASM_OFFSETS_HEADER) $(CLASSPATH_CONFIG) $(LIB_OBJS)
	$(E) "  AR      " $@
	$(Q) rm -f $@ && $(AR) rcs $@ $(LIB_OBJS)

check-unit: arch/$(ARCH)/insn-selector$(ARCH_POSTFIX).c
	+$(MAKE) -C test/unit/vm/ SYS=$(SYS) ARCH=$(ARCH) ARCH_POSTFIX=$(ARCH_POSTFIX) $(TEST)
	+$(MAKE) -C test/unit/arch-$(ARCH)/ SYS=$(SYS) ARCH=$(ARCH) ARCH_POSTFIX=$(ARCH_POSTFIX) $(TEST)
.PHONY: check-unit

check-integration: $(LIB_FILE)
	+$(MAKE) -C test/integration check
.PHONY: check-integration

JAVA_TESTS += test/functional/jato/internal/VM.java
JAVA_TESTS += test/functional/jvm/ArgsTest.java
JAVA_TESTS += test/functional/jvm/ArrayExceptionsTest.java
JAVA_TESTS += test/functional/jvm/ArrayMemberTest.java
JAVA_TESTS += test/functional/jvm/ArrayTest.java
JAVA_TESTS += test/functional/jvm/BranchTest.java
JAVA_TESTS += test/functional/jvm/CFGCrashTest.java
JAVA_TESTS += test/functional/jvm/ClassExceptionsTest.java
JAVA_TESTS += test/functional/jvm/ClassLoaderTest.java
JAVA_TESTS += test/functional/jvm/ClinitFloatTest.java
JAVA_TESTS += test/functional/jvm/CloneTest.java
JAVA_TESTS += test/functional/jvm/ControlTransferTest.java
JAVA_TESTS += test/functional/jvm/ConversionTest.java
JAVA_TESTS += test/functional/jvm/DoubleArithmeticTest.java
JAVA_TESTS += test/functional/jvm/DoubleConversionTest.java
JAVA_TESTS += test/functional/jvm/ExceptionsTest.java
JAVA_TESTS += test/functional/jvm/ExitStatusIsOneTest.java
JAVA_TESTS += test/functional/jvm/ExitStatusIsZeroTest.java
JAVA_TESTS += test/functional/jvm/FibonacciTest.java
JAVA_TESTS += test/functional/jvm/FinallyTest.java
JAVA_TESTS += test/functional/jvm/FloatArithmeticTest.java
JAVA_TESTS += test/functional/jvm/FloatConversionTest.java
JAVA_TESTS += test/functional/jvm/GcTortureTest.java
JAVA_TESTS += test/functional/jvm/GetstaticPatchingTest.java
JAVA_TESTS += test/functional/jvm/IntegerArithmeticExceptionsTest.java
JAVA_TESTS += test/functional/jvm/IntegerArithmeticTest.java
JAVA_TESTS += test/functional/jvm/InterfaceFieldInheritanceTest.java
JAVA_TESTS += test/functional/jvm/InterfaceInheritanceTest.java
JAVA_TESTS += test/functional/jvm/InvokeinterfaceTest.java
JAVA_TESTS += test/functional/jvm/InvokestaticPatchingTest.java
JAVA_TESTS += test/functional/jvm/LoadConstantsTest.java
JAVA_TESTS += test/functional/jvm/LongArithmeticExceptionsTest.java
JAVA_TESTS += test/functional/jvm/LongArithmeticTest.java
JAVA_TESTS += test/functional/jvm/MethodInvocationAndReturnTest.java
JAVA_TESTS += test/functional/jvm/MethodInvocationExceptionsTest.java
JAVA_TESTS += test/functional/jvm/MethodInvokeVirtualTest.java
JAVA_TESTS += test/functional/jvm/MonitorTest.java
JAVA_TESTS += test/functional/jvm/MultithreadingTest.java
JAVA_TESTS += test/functional/jvm/ObjectArrayTest.java
JAVA_TESTS += test/functional/jvm/ObjectCreationAndManipulationExceptionsTest.java
JAVA_TESTS += test/functional/jvm/ObjectCreationAndManipulationTest.java
JAVA_TESTS += test/functional/jvm/ObjectStackTest.java
JAVA_TESTS += test/functional/jvm/ParameterPassingLivenessTest.java
JAVA_TESTS += test/functional/jvm/ParameterPassingTest.java
JAVA_TESTS += test/functional/jvm/PrintTest.java
JAVA_TESTS += test/functional/jvm/PutfieldTest.java
JAVA_TESTS += test/functional/jvm/PutstaticPatchingTest.java
JAVA_TESTS += test/functional/jvm/PutstaticTest.java
JAVA_TESTS += test/functional/jvm/RegisterAllocatorTortureTest.java
JAVA_TESTS += test/functional/jvm/StackTraceTest.java
JAVA_TESTS += test/functional/jvm/StringTest.java
JAVA_TESTS += test/functional/jvm/SwitchTest.java
JAVA_TESTS += test/functional/jvm/SynchronizationExceptionsTest.java
JAVA_TESTS += test/functional/jvm/SynchronizationTest.java
JAVA_TESTS += test/functional/jvm/TestCase.java
JAVA_TESTS += test/functional/jvm/TrampolineBackpatchingTest.java
JAVA_TESTS += test/functional/jvm/VirtualAbstractInterfaceMethodTest.java
JAVA_TESTS += test/functional/test/java/lang/ClassTest.java
JAVA_TESTS += test/functional/test/java/lang/DoubleTest.java
JAVA_TESTS += test/functional/test/java/lang/JNITest.java
JAVA_TESTS += test/functional/test/java/lang/ref/ReferenceTest.java
JAVA_TESTS += test/functional/test/java/lang/reflect/FieldAccessorsTest.java
JAVA_TESTS += test/functional/test/java/lang/reflect/FieldTest.java
JAVA_TESTS += test/functional/test/java/lang/reflect/MethodTest.java
JAVA_TESTS += test/functional/test/java/util/HashMapTest.java
JAVA_TESTS += test/functional/test/sun/misc/UnsafeTest.java

JASMIN_TESTS += test/functional/jvm/DupTest.j
JASMIN_TESTS += test/functional/jvm/EntryTest.j
JASMIN_TESTS += test/functional/jvm/ExceptionHandlerTest.j
JASMIN_TESTS += test/functional/jvm/InvokeResultTest.j
JASMIN_TESTS += test/functional/jvm/InvokeTest.j
JASMIN_TESTS += test/functional/jvm/MethodOverridingFinal.j
JASMIN_TESTS += test/functional/jvm/NoSuchMethodErrorTest.j
JASMIN_TESTS += test/functional/jvm/PopTest.j
JASMIN_TESTS += test/functional/jvm/SubroutineTest.j
JASMIN_TESTS += test/functional/jvm/WideTest.j

MBENCH_TEST_SUITE_CLASSES = test/perf/ICTime.java

compile-java-tests: $(PROGRAMS) FORCE
	$(E) "  JAVAC   " $(JAVA_TESTS)
	$(Q) JAVA=$(JAVA) $(JAVAC) $(JAVAC_OPTS) -cp $(GLIBJ):test/functional -d test/functional $(JAVA_TESTS)
.PHONY: compile-java-tests

compile-mbench-tests: $(PROGRAMS) FORCE
	$(E) "  JAVAC   " $(MBENCH_TEST_SUITE_CLASSES)
	$(Q) JAVA=$(JAVA) $(JAVAC) $(JAVAC_OPTS) -cp $(GLIBJ):test/perf -d test/perf $(MBENCH_TEST_SUITE_CLASSES)
.PHONY: compile-mbench-tests

compile-jasmin-tests: $(PROGRAMS) FORCE
	$(E) "  JASMIN  " $(JASMIN_TESTS)
	$(Q) $(JASMIN) $(JASMIN_OPTS) -d test/functional $(JASMIN_TESTS) > /dev/null
.PHONY: compile-jasmin-tests

lib: $(CLASSPATH_CONFIG)
	+$(MAKE) -C lib/ JAVAC=$(JAVAC) GLIBJ=$(GLIBJ)
.PHONY: lib

compile-jni-test-lib: $(PROGRAMS)
	+$(MAKE) -C test/functional/jni CC='$(CC)' JAVA='$(JAVA)' JAVAC='$(JAVAC)'
.PHONY: compile-jni-test-lib

check-functional: monoburg $(CLASSPATH_CONFIG) $(PROGRAMS) compile-java-tests compile-jasmin-tests compile-jni-test-lib
	$(E) "  REGRESSION"
	$(Q) ./tools/test.py
.PHONY: check-functional

check-mbench: monoburg $(CLASSPATH_CONFIG) $(PROGRAMS) compile-mbench-tests
	$(E) "  MICROBENCHMARKS"
	$(Q) for i in $(patsubst %.java,%,$(MBENCH_TEST_SUITE_CLASSES))\
	;do \
		echo "MBENCH "$$i; $(JAVA) -classpath test/perf: $$i \
	;done
.PHONY: check-mbench

check: check-unit check-integration check-functional
.PHONY: check

torture:
	$(E) "  MAKE"
	$(Q) $(MAKE) -C torture JAVA=$(JAVA)
.PHONY: torture

clean:
	$(E) "  CLEAN"
	$(Q) - rm -f $(PROGRAMS)
	$(Q) - rm -f $(LIB_FILE)
	$(Q) - rm -f $(VERSION_HEADER)
	$(Q) - rm -f $(CLASSPATH_CONFIG)
	$(Q) - rm -f $(OBJS)
	$(Q) - rm -f $(OBJS:.o=.d)
	$(Q) - rm -f $(LIB_OBJS)
	$(Q) - rm -f $(LIB_OBJS:.o=.d)
	$(Q) - rm -f $(ARCH_TEST_OBJS)
	$(Q) - rm -f arch/$(ARCH)/insn-selector.c
	$(Q) - rm -f $(ARCH_TEST_SUITE)
	$(Q) - rm -f test-suite.o
	$(Q) - rm -f $(ARCH_TESTRUNNER)
	$(Q) - rm -f $(RUNTIME_CLASSES)
	$(Q) - find test/functional/ -name "*.class" | grep -v corrupt | xargs rm -f
	$(Q) - find runtime/ -name "*.class" | xargs rm -f
	$(Q) - rm -f tags
	+$(Q) - $(MAKE) -C tools/monoburg/ clean >/dev/null
	+$(Q) - $(MAKE) -C boehmgc/ clean >/dev/null
	+$(Q) - $(MAKE) -C test/functional/jni/ clean >/dev/null
	+$(Q) - $(MAKE) -C test/unit/vm/ clean >/dev/null
	+$(Q) - $(MAKE) -C test/unit/arch-$(ARCH)/ clean >/dev/null
	+$(Q) - $(MAKE) -C test/integration/ clean >/dev/null
.PHONY: clean

INSTALL_PREFIX	?= $(HOME)

install: jato
	$(E) "  INSTALL " jato
	$(Q) $(INSTALL) -d -m 755 $(INSTALL_PREFIX)/bin
	$(Q) $(INSTALL) jato $(INSTALL_PREFIX)/bin
	$(Q) $(INSTALL) tools/ecj-jato/ecj-3.7.2.jar $(INSTALL_PREFIX)/bin
	$(Q) $(INSTALL) tools/ecj-jato/ecj-jato $(INSTALL_PREFIX)/bin
.PHONY: install

tags: FORCE
	$(E) "  TAGS"
	$(Q) rm -f tags
	$(Q) find . -name '*.[hcS]' -print | xargs ctags -a

PHONY += FORCE
FORCE:

include scripts/build/common.mk

-include $(OBJS:.o=.d)
