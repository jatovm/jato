CCFLAGS = -rdynamic -g -Wall -Wundef -Wsign-compare -Os -std=gnu99
INCLUDE=-I. -I../libharness -I../jit -I../../include -I../include -I../../jit/glib
LIBS = -lpthread -lm -ldl -lz -lbfd -lopcodes
MONOBURG = ../../monoburg/monoburg

include ../../scripts/Makefile.include

quiet_cmd_cc_o_c = CC $(empty)     $(empty) $@
      cmd_cc_o_c = $(CC) $(CCFLAGS) $(INCLUDE) $(DEFINES) -c $< -o `basename $@`

%.o: %.c
	$(call cmd,cc_o_c)

RUNNER=i386-test-runner
SUITE=i386-test-suite.c

OBJS = \
	../libharness/libharness.o \
	../jamvm/alloc-stub.o \
	../../vm/backtrace.o \
	../../vm/buffer.o \
	../../vm/stack.o \
	../../vm/string.o \
	../../vm/types.o \
	../../jit/alloc.o \
	../../jit/basic-block.o \
	../../jit/compilation-unit.o \
	../../jit/emit.o \
	../../jit/expression.o \
	../../jit/statement.o \
	../../jit/tree-printer.o \
	../../arch/i386/emit-code.o \
	../../arch/i386/instruction.o \
	../../arch/i386/insn-selector.o \
	../../arch/i386/stack-frame.o \
	../../arch/i386/use-def.o \
	emit-code-test.o \
	insn-selector-test.o \
	stack-frame-test.o \
	use-def-test.o \

quiet_cmd_run_tests = RUNTEST $(empty) $(RUNNER)
      cmd_run_tests = ./$(RUNNER)

quiet_cmd_cc_test_runner = MAKE $(empty)   $(empty) $(RUNNER)
      cmd_cc_test_runner = $(CC) $(CCFLAGS) $(INCLUDE) $(LIBS) *.o $(SUITE) -o $(RUNNER)

test: $(SUITE) $(OBJS)
	$(call cmd,cc_test_runner)
	$(call cmd,run_tests)

../../arch/i386/insn-selector.c: FORCE
	$(E) "  MONOBURG" $@
	$(Q) $(MONOBURG) -p -e ../../arch/i386/insn-selector.brg > $@

$(OBJS): FORCE

quiet_cmd_gen_suite = SUITE $(empty)  $(empty) $@
      cmd_gen_suite = sh ../scripts/make-tests.sh *.c > $@

$(SUITE): FORCE
	$(call cmd,gen_suite)

quiet_cmd_clean = CLEAN
      cmd_clean = rm -f *.o $(RUNNER) $(SUITE)

clean: FORCE
	$(call cmd,clean)

PHONY += FORCE
FORCE:
# DO NOT DELETE
