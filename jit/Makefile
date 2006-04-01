TESTRUNNER=test-runner
TEST_SUITE=test-suite.c

MONOBURG=../monoburg/monoburg
CC = gcc
CCFLAGS = -rdynamic -g -Wall -Wundef -Wsign-compare -Werror -Os -std=gnu99
INCLUDE = -I. -I../src -I./glib -I../include
LIBS =
OBJS = alloc.o basic-block.o bitmap.o bytecodes.o bytecode-arithmetic-converters.o \
	bytecode-cfg-builder.o bytecode-to-ir.o compilation-unit.o \
	jit-compiler.o instruction.o insn-selector.o stack.o statement.o \
	expression.o x86-objcode.o jit.o
TESTS = basic-block-test.o bitmap-test.o bytecodes-test.o \
	bytecode-arithmetic-converters-test.o bytecode-cfg-builder-test.o \
	bytecode-converter-fixture.o bytecode-to-ir-test.o \
	compilation-unit-test.o insn-selector-test.o \
	jit-compiler-test.o list-test.o stack-test.o x86-objcode-test.o \
	jit-test.o
HARNESS = libharness.o backtrace.o

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

# If quiet is set, only print short version of command
cmd = @$(if $($(quiet)cmd_$(1)),\
      echo '  $($(quiet)cmd_$(1))' &&) $(cmd_$(1))

quiet_cmd_cc_o_c = CC $(empty)     $(empty) $@
      cmd_cc_o_c = $(CC) $(CCFLAGS) $(INCLUDE) -c $< -o $@

%.o: %.c
	$(call cmd,cc_o_c)

all: test

ARCH_H = ../include/vm/arch.h

quiet_cmd_ln_arch_h = LN $(empty)     $(empty) $@
      cmd_ln_arch_h = ln -fsn ../../src/arch/i386.h $@

$(ARCH_H):
	$(call cmd,ln_arch_h)

$(OBJS): $(ARCH_H)

$(TESTS):

$(HARNESS):

quiet_cmd_monoburg = MONOBURG $@
      cmd_monoburg = $(MONOBURG) -p -e $< > $@

insn-selector.c: insn-selector.brg
	$(call cmd,monoburg)

quiet_cmd_tags = TAGS
      cmd_tags = exuberant-ctags -R

tags:
	$(call cmd,tags)

quiet_cmd_gensuite = GENSUITE $@
      cmd_gensuite = sh make-tests.sh > $@

test-suite.c:
	$(call cmd,gensuite)

quiet_cmd_runtests = RUNTESTS $(TESTRUNNER)
      cmd_runtests = ./$(TESTRUNNER)

quiet_cmd_cc_testrunner = MAKE $(empty)   $(empty) $(TESTRUNNER)
      cmd_cc_testrunner = $(CC) $(CCFLAGS) $(INCLUDE) $(TEST_SUITE) $(LIBS) $(OBJS) $(TESTS) $(HARNESS) -o $(TESTRUNNER)

test: $(OBJS) $(TESTS) $(HARNESS) test-suite.c
	$(call cmd,cc_testrunner)
	$(call cmd,runtests)

quiet_cmd_clean = CLEAN
      cmd_clean = rm -rf insn-selector.c $(TESTRUNNER) $(TEST_SUITE) *.o tags

clean:
	$(call cmd,clean)
