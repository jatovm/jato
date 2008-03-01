CCFLAGS = -rdynamic -g -Wall -Wundef -Wsign-compare -Os -std=gnu99
INCLUDE=-I. -I../libharness -I../jit -I../../include -I../include -I../../jit/glib
LIBS = -lpthread -lm -ldl -lz -lbfd -lopcodes
MONOBURG = ../../monoburg/monoburg

include ../../scripts/Makefile.include

quiet_cmd_cc_o_c = CC $(empty)     $(empty) $@
      cmd_cc_o_c = $(CC) $(CCFLAGS) $(INCLUDE) $(DEFINES) -c $< -o `basename $@`

%.o: %.c
	$(call cmd,cc_o_c)

RUNNER=x86-test-runner
SUITE=x86-test-suite.c

OBJS = \
	../libharness/libharness.o \
	../jamvm/alloc-stub.o \
	../../vm/backtrace.o \
	../../vm/buffer.o \
	../../vm/stack.o \
	../../vm/string.o \
	../../vm/types.o \
	../../vm/zalloc.o \
	../../jit/alloc.o \
	../../jit/basic-block.o \
	../../jit/compilation-unit.o \
	../../jit/emit.o \
	../../jit/expression.o \
	../../jit/interval.o \
	../../jit/stack-slot.o \
	../../jit/statement.o \
	../../jit/tree-printer.o \
	../../arch/x86/emit-code.o \
	../../arch/x86/instruction.o \
	../../arch/x86/insn-selector.o \
	../../arch/x86/stack-frame.o \
	../../arch/x86/use-def.o \
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

../../arch/x86/insn-selector.c: FORCE
	$(E) "  MONOBURG" $@
	$(Q) $(MONOBURG) -p -e ../../arch/x86/insn-selector.brg > $@

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
