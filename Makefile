MONOBURG=./monoburg/monoburg
CC = gcc
CCFLAGS = -rdynamic -g -Wall -Wundef -Wsign-compare -Os -std=gnu99
DEFINES = -DINSTALL_DIR=\"$(prefix)\" -DCLASSPATH_INSTALL_DIR=\"$(with_classpath_install_dir)\"
INCLUDE =  -Iinclude -Ijit -Isrc -Ijit/glib -Itest/libharness -Itest/jit
LIBS = -lpthread -lm -ldl -lz -lbfd -lopcodes

ARCH_H = include/vm/arch.h
EXECUTABLE = jato-exe

# GNU classpath installation path
prefix = /usr/local
with_classpath_install_dir = /usr/local/classpath

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
	jit/bytecodes.o \
	jit/cfg-analyzer.o \
	jit/compilation-unit.o \
	jit/disass-common.o \
	jit/disass.o \
	jit/expression.o \
	jit/insn-selector.o \
	jit/instruction.o \
	jit/invoke-bc.o \
	jit/jit-compiler.o \
	jit/jvm_types.o \
	jit/load-store-bc.o \
	jit/object-bc.o \
	jit/ostack-bc.o \
	jit/statement.o \
	jit/tree-printer.o \
	jit/typeconv-bc.o \
	jit/x86-frame.o \
	jit/x86-objcode.o \
	vm/backtrace.o \
	vm/bitmap.o \
	vm/buffer.o \
	vm/debug-dump.o \
	vm/natives.o \
	vm/stack.o \
	vm/string.o

JAMVM_OBJS = \
	jato/jato.o \
	src/access.o \
	src/alloc.o \
	src/cast.o \
	src/class.o \
	src/direct.o \
	src/dll.o \
	src/dll_ffi.o \
	src/excep.o \
	src/execute.o \
	src/frame.o \
	src/hash.o \
	src/interp.o \
	src/jni.o \
	src/lock.o \
	src/natives.o \
	src/os/linux/i386/dll_md.o \
	src/os/linux/i386/init.o \
	src/os/linux/os.o \
	src/properties.o \
	src/reflect.o \
	src/resolve.o \
	src/string.o \
	src/thread.o \
	src/utf8.o \
	src/zip.o
	
# If quiet is set, only print short version of command
cmd = @$(if $($(quiet)cmd_$(1)),\
      echo '  $($(quiet)cmd_$(1))' &&) $(cmd_$(1))

quiet_cmd_cc_o_c = CC $(empty)     $(empty) $@
      cmd_cc_o_c = $(CC) $(CCFLAGS) $(INCLUDE) $(DEFINES) -c $< -o $@

%.o: %.c
	$(call cmd,cc_o_c)

all: $(EXECUTABLE)

quiet_cmd_ln_arch_h = LN $(empty)     $(empty) $@
      cmd_ln_arch_h = ln -fsn ../../src/arch/i386.h $@

$(ARCH_H):
	$(call cmd,ln_arch_h)

gen:
	$(MONOBURG) -p -e jit/insn-selector.brg > jit/insn-selector.c

quiet_cmd_cc_exec = LD $(empty)     $(empty) $(EXECUTABLE)
      cmd_cc_exec = $(CC) $(CCFLAGS) $(INCLUDE) $(DEFINES) $(LIBS) $(JAMVM_OBJS) $(JATO_OBJS) -o $(EXECUTABLE)

$(EXECUTABLE): $(ARCH_H) gen compile
	$(call cmd,cc_exec)

compile: $(JAMVM_OBJS) $(JATO_OBJS)

TESTRUNNER=test-runner
TEST_SUITE=test-suite.c

TEST_OBJS = \
	test/jit/arithmetic-bc-test.o \
	test/jit/basic-block-test.o \
	test/jit/bc-test-utils.o \
	test/jit/branch-bc-test.o \
	test/jit/bytecode-to-ir-test.o \
	test/jit/bytecodes-test.o \
	test/jit/cfg-analyzer-test.o \
	test/jit/compilation-unit-test.o \
	test/jit/expression-test.o \
	test/jit/insn-selector-test.o \
	test/jit/invoke-bc-test.o \
	test/jit/jit-compiler-test.o \
	test/jit/object-bc-test.o \
	test/jit/resolve-stub.o \
	test/jit/tree-printer-test.o \
	test/jit/x86-frame-test.o \
	test/jit/x86-objcode-test.o \
	test/libharness/libharness.o \
	test/vm/bitmap-test.o \
	test/vm/buffer-test.o \
	test/vm/list-test.o \
	test/vm/natives-test.o \
	test/vm/stack-test.o \
	test/vm/string-test.o \

$(TEST_OBJS):

quiet_cmd_gensuite = GENSUITE $@
      cmd_gensuite = sh test/scripts/make-tests.sh > $@

test-suite.c:
	$(call cmd,gensuite)

quiet_cmd_runtests = RUNTEST $(TESTRUNNER)
      cmd_runtests = ./$(TESTRUNNER)

quiet_cmd_cc_testrunner = MAKE $(empty)   $(empty) $(TESTRUNNER)
      cmd_cc_testrunner = $(CC) $(CCFLAGS) $(INCLUDE) $(TEST_SUITE) $(LIBS) $(JATO_OBJS) $(TEST_OBJS) $(HARNESS) -o $(TESTRUNNER)

test: gen $(ARCH_H) $(JATO_OBJS) $(TEST_OBJS) $(HARNESS) test-suite.c
	$(call cmd,cc_testrunner)
	$(call cmd,runtests)

quiet_cmd_jikes_o_c = JIKES $(empty)  $(empty) $@
      cmd_jikes_o_c = $(JIKES) -cp $(BOOTCLASSPATH) -d acceptance $<

%.class: %.java
	$(call cmd,jikes_o_c)

ACCEPTANCE_CLASSES = \
	acceptance/jamvm/ExitStatusIsOneTest.class \
	acceptance/jamvm/ExitStatusIsZeroTest.class \
	acceptance/jamvm/IntegerArithmeticTest.class

vm-classes:
	make -C lib/

acceptance: vm-classes $(EXECUTABLE) $(ACCEPTANCE_CLASSES)

clean:
	rm -f $(JAMVM_OBJS)
	rm -f $(JATO_OBJS)
	rm -f $(TEST_OBJS)
	rm -f jit/insn-selector.c
	rm -f $(EXECUTABLE)
	rm -f $(ARCH_H)
	rm -f test-suite.c
	rm -f test-suite.o
	rm -f test-runner
