ARCH := $(shell uname -m | sed  -e s/i.86/i386/)
OS := $(shell uname -s | tr "[:upper:]" "[:lower:]")

include arch/$(ARCH)/Makefile

MONOBURG=./monoburg/monoburg
CC = gcc
CCFLAGS = -rdynamic -g -Wall -Wundef -Wsign-compare -Os -std=gnu99
DEFINES = -DINSTALL_DIR=\"$(prefix)\" -DCLASSPATH_INSTALL_DIR=\"$(with_classpath_install_dir)\"
INCLUDE =  -Iinclude -Ijit -Ijamvm -Ijit/glib -Itest/libharness -Itest/jit -include arch/config.h $(ARCH_INCLUDE)
LIBS = -lpthread -lm -ldl -lz -lbfd -lopcodes $(ARCH_LIBS)

ARCH_INCLUDE_DIR = include/arch
ARCH_H = include/vm/arch.h
EXECUTABLE = java

# GNU classpath installation path
prefix = /usr/local
with_classpath_install_dir = /usr/

JIKES=jikes
GLIBJ=$(with_classpath_install_dir)/share/classpath/glibj.zip
BOOTCLASSPATH=lib/classes.zip:$(GLIBJ)

# Default to quiet output and add verbose printing only if V=1 is passed
# as parameter to make.
ifdef V
  ifeq ("$(origin V)", "command line")
    BUILD_VERBOSE = $(V)
  endif
endif

ifndef BUILD_VERBOSE
  BUILD_VERBOSE = 0
endif

ifeq ($(BUILD_VERBOSE), 1)
  quiet =
else
  quiet = quiet_
endif

ARCH_OBJS = \
	arch/$(ARCH)/emit-code.o \
	arch/$(ARCH)/instruction.o \
	arch/$(ARCH)/insn-selector.o \
	arch/$(ARCH)/stack-frame.o \
	arch/$(ARCH)/use-def.o

JIT_OBJS = \
	jit/alloc.o \
	jit/arithmetic-bc.o \
	jit/basic-block.o \
	jit/branch-bc.o \
	jit/bytecode-to-ir.o \
	jit/cfg-analyzer.o \
	jit/compilation-unit.o \
	jit/disass-common.o \
	jit/disass.o \
	jit/emit.o \
	jit/expression.o \
	jit/invoke-bc.o \
	jit/jit-compiler.o \
	jit/linear-scan.o \
	jit/liveness.o \
	jit/load-store-bc.o \
	jit/object-bc.o \
	jit/ostack-bc.o \
	jit/statement.o \
	jit/trace-jit.o \
	jit/tree-printer.o \
	jit/typeconv-bc.o \

VM_OBJS = \
	vm/backtrace.o \
	vm/bitset.o \
	vm/buffer.o \
	vm/bytecodes.o \
	vm/debug-dump.o \
	vm/natives.o \
	vm/stack.o \
	vm/string.o \
	vm/types.o

JAMVM_OBJS = \
	jato/jato.o \
	jamvm/access.o \
	jamvm/alloc.o \
	jamvm/cast.o \
	jamvm/class.o \
	jamvm/direct.o \
	jamvm/dll.o \
	jamvm/dll_ffi.o \
	jamvm/excep.o \
	jamvm/execute.o \
	jamvm/frame.o \
	jamvm/hash.o \
	jamvm/hooks.o \
	jamvm/init.o \
	jamvm/interp.o \
	jamvm/jni.o \
	jamvm/lock.o \
	jamvm/natives.o \
	jamvm/os/$(OS)/i386/dll_md.o \
	jamvm/os/$(OS)/i386/init.o \
	jamvm/os/$(OS)/os.o \
	jamvm/properties.o \
	jamvm/reflect.o \
	jamvm/resolve.o \
	jamvm/string.o \
	jamvm/thread.o \
	jamvm/utf8.o \
	jamvm/zip.o

JATO_OBJS = $(ARCH_OBJS) $(JIT_OBJS) $(VM_OBJS)

OBJS = $(JAMVM_OBJS) $(JATO_OBJS)
	
# If quiet is set, only print short version of command
cmd = @$(if $($(quiet)cmd_$(1)),\
      echo '  $($(quiet)cmd_$(1))' &&) $(cmd_$(1))

quiet_cmd_cc_o_c = CC $(empty)     $(empty) $@
      cmd_cc_o_c = $(CC) $(CCFLAGS) $(INCLUDE) $(DEFINES) -c $< -o $@

%.o: %.c
	$(call cmd,cc_o_c)

all: $(EXECUTABLE) test

quiet_cmd_ln_arch_h = LN $(empty)     $(empty) $@
      cmd_ln_arch_h = ln -fsn ../../jamvm/arch/$(ARCH).h $@

$(ARCH_H):
	$(call cmd,ln_arch_h)

quiet_cmd_monoburg_exec = MONOBURG $@
      cmd_monoburg_exec = $(MONOBURG) -p -e arch/$(ARCH)/insn-selector.brg > $@

arch/$(ARCH)/insn-selector.c: FORCE
	$(call cmd,monoburg_exec)

quiet_cmd_cc_exec = LD $(empty)     $(empty) $(EXECUTABLE)
      cmd_cc_exec = $(CC) $(CCFLAGS) $(INCLUDE) $(DEFINES) $(LIBS) $(OBJS) -o $(EXECUTABLE)

$(EXECUTABLE): $(ARCH_INCLUDE_DIR) $(ARCH_H) compile
	$(call cmd,cc_exec)

compile: $(OBJS)

LIBHARNESS_OBJS = \
	test/libharness/libharness.o

quiet_cmd_ln_arch = LN $(empty)     $(empty) $@
      cmd_ln_arch = ln -fsn arch-$(ARCH) $@

$(ARCH_INCLUDE_DIR): FORCE
	$(call cmd,ln_arch)

quiet_cmd_arch_gensuite = GENSUITE $@
      cmd_arch_gensuite = sh test/scripts/make-tests.sh test/arch-$(ARCH)/*.c > $@

test: $(ARCH_INCLUDE_DIR) $(ARCH_H) FORCE
	make -C test/vm/ test
	make -C test/jit/ test
	make -C test/arch-i386/ test

quiet_cmd_jikes = JIKES $(empty)  $(empty) $@
      cmd_jikes = $(JIKES) -cp $(BOOTCLASSPATH):test/regression -d test/regression $<

%.class: %.java
	$(call cmd,jikes)

REGRESSION_TEST_SUITE_CLASSES = \
	test/regression/jamvm/TestCase.class \
	test/regression/jamvm/ExitStatusIsOneTest.class \
	test/regression/jamvm/ExitStatusIsZeroTest.class \
	test/regression/jamvm/LoadConstantsTest.class \
	test/regression/jamvm/IntegerArithmeticTest.class \
	test/regression/jamvm/ObjectCreationAndManipulationTest.class \
	test/regression/jamvm/MethodInvocationAndReturnTest.class

vm-classes:
	make -C lib/

compile-regression-suite: vm-classes $(EXECUTABLE) $(REGRESSION_TEST_SUITE_CLASSES)

quiet_cmd_clean = CLEAN
      cmd_clean = rm -f $(OBJS) $(LIBHARNESS_OBJS) $(ARCH_TEST_OBJS) \
			arch/$(ARCH)/insn-selector.c $(EXECUTABLE) $(ARCH_H) \
			$(ARCH_TEST_SUITE) \
			test-suite.o $(ARCH_TESTRUNNER) \
			$(REGRESSION_TEST_SUITE_CLASSES) \
			tags include/arch

clean: FORCE
	$(call cmd,clean)
	make -C test/vm/ clean
	make -C test/jit/ clean
	make -C test/arch-i386/ clean

quiet_cmd_tags = TAGS
      cmd_tags = exuberant-ctags -R

tags: FORCE
	$(call cmd,tags)

PHONY += FORCE
FORCE:
