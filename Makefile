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

JATO_OBJS =  \
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
	jit/insn-selector.o \
	jit/instruction.o \
	jit/invoke-bc.o \
	jit/jit-compiler.o \
	jit/load-store-bc.o \
	jit/object-bc.o \
	jit/ostack-bc.o \
	jit/statement.o \
	jit/trace-jit.o \
	jit/tree-printer.o \
	jit/typeconv-bc.o \
	jit/use-def.o \
	jit/x86-frame.o \
	jit/x86-objcode.o \
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
      cmd_monoburg_exec = $(MONOBURG) -p -e jit/insn-selector.brg > $@

jit/insn-selector.c: FORCE
	$(call cmd,monoburg_exec)

quiet_cmd_cc_exec = LD $(empty)     $(empty) $(EXECUTABLE)
      cmd_cc_exec = $(CC) $(CCFLAGS) $(INCLUDE) $(DEFINES) $(LIBS) $(JAMVM_OBJS) $(JATO_OBJS) -o $(EXECUTABLE)

$(EXECUTABLE): $(ARCH_INCLUDE_DIR) $(ARCH_H) compile
	$(call cmd,cc_exec)

compile: $(JAMVM_OBJS) $(JATO_OBJS)

TESTRUNNER=test-runner
TEST_SUITE=test-suite.c

TEST_OBJS = \
	test/jit/alloc-stub.o \
	test/jit/arithmetic-bc-test.o \
	test/jit/basic-block-test.o \
	test/jit/bc-test-utils.o \
	test/jit/branch-bc-test.o \
	test/jit/bytecode-to-ir-test.o \
	test/jit/cfg-analyzer-test.o \
	test/jit/compilation-unit-test.o \
	test/jit/expression-test.o \
	test/jit/insn-selector-test.o \
	test/jit/invoke-bc-test.o \
	test/jit/jit-compiler-test.o \
	test/jit/load-store-bc-test.o \
	test/jit/object-bc-test.o \
	test/jit/ostack-bc-test.o \
	test/jit/resolve-stub.o \
	test/jit/tree-printer-test.o \
	test/jit/typeconv-bc-test.o \
	test/jit/use-def-test.o \
	test/jit/x86-frame-test.o \
	test/jit/x86-objcode-test.o \
	test/libharness/libharness.o \
	test/vm/bitset-test.o \
	test/vm/buffer-test.o \
	test/vm/bytecodes-test.o \
	test/vm/list-test.o \
	test/vm/natives-test.o \
	test/vm/stack-test.o \
	test/vm/string-test.o \

$(TEST_OBJS):

quiet_cmd_ln_arch = LN $(empty)     $(empty) $@
      cmd_ln_arch = ln -fsn arch-$(ARCH) $@

$(ARCH_INCLUDE_DIR): FORCE
	$(call cmd,ln_arch)

quiet_cmd_gensuite = GENSUITE $@
      cmd_gensuite = sh test/scripts/make-tests.sh > $@

test-suite.c: FORCE
	$(call cmd,gensuite)

quiet_cmd_runtests = RUNTEST $(TESTRUNNER)
      cmd_runtests = ./$(TESTRUNNER)

quiet_cmd_cc_testrunner = MAKE $(empty)   $(empty) $(TESTRUNNER)
      cmd_cc_testrunner = $(CC) $(CCFLAGS) $(INCLUDE) $(TEST_SUITE) $(LIBS) $(JATO_OBJS) $(TEST_OBJS) $(HARNESS) -o $(TESTRUNNER)

test: $(ARCH_INCLUDE_DIR) $(ARCH_H) $(JATO_OBJS) $(TEST_OBJS) $(HARNESS) test-suite.c
	$(call cmd,cc_testrunner)
	$(call cmd,runtests)

quiet_cmd_jikes = JIKES $(empty)  $(empty) $@
      cmd_jikes = $(JIKES) -cp $(BOOTCLASSPATH):acceptance -d acceptance $<

%.class: %.java
	$(call cmd,jikes)

ACCEPTANCE_CLASSES = \
	acceptance/jamvm/TestCase.class \
	acceptance/jamvm/ExitStatusIsOneTest.class \
	acceptance/jamvm/ExitStatusIsZeroTest.class \
	acceptance/jamvm/LoadConstantsTest.class \
	acceptance/jamvm/IntegerArithmeticTest.class \
	acceptance/jamvm/ObjectCreationAndManipulationTest.class

vm-classes:
	make -C lib/

acceptance: vm-classes $(EXECUTABLE) $(ACCEPTANCE_CLASSES)

quiet_cmd_clean = CLEAN
      cmd_clean = rm -f $(JAMVM_OBJS) $(JATO_OBJS) $(TEST_OBJS) \
      			jit/insn-selector.c $(EXECUTABLE) $(ARCH_H) \
			test-suite.c test-suite.o test-runner \
			$(ACCEPTANCE_CLASSES) tags include/arch

clean: FORCE
	$(call cmd,clean)

quiet_cmd_tags = TAGS
      cmd_tags = exuberant-ctags -R

tags: FORCE
	$(call cmd,tags)

PHONY += FORCE
FORCE:
