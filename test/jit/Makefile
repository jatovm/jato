CCFLAGS = -rdynamic -g -Wall -Wundef -Wsign-compare -Os -std=gnu99
INCLUDE=-I../include/ -I. -I../libharness -I../../include -I../../jit/glib
LIBS = -lpthread -lm -ldl -lz -lbfd -lopcodes -liberty

include ../../scripts/Makefile.include

quiet_cmd_cc_o_c = CC $(empty)     $(empty) $@
      cmd_cc_o_c = $(CC) $(CCFLAGS) $(INCLUDE) $(DEFINES) -c $< -o `basename $@`

%.o: %.c
	$(call cmd,cc_o_c)

RUNNER=jit-test-runner
SUITE=jit-test-suite.c

OBJS = \
	../../arch/$(ARCH)/backtrace.o \
	../../vm/bitset.o \
	../../vm/buffer.o \
	../../vm/bytecodes.o \
	../../vm/stack.o \
	../../vm/string.o \
	../../vm/types.o \
	../../vm/zalloc.o \
	../../jit/arithmetic-bc.o \
	../../jit/basic-block.o \
	../../jit/branch-bc.o \
	../../jit/bytecode-to-ir.o \
	../../jit/cfg-analyzer.o \
	../../jit/compilation-unit.o \
	../../jit/expression.o \
	../../jit/interval.o \
	../../jit/invoke-bc.o \
	../../jit/linear-scan.o \
	../../jit/liveness.o \
	../../jit/load-store-bc.o \
	../../jit/object-bc.o \
	../../jit/ostack-bc.o \
	../../jit/stack-slot.o \
	../../jit/statement.o \
	../../jit/spill-reload.o \
	../../jit/typeconv-bc.o \
	../../jit/tree-printer.o \
	../libharness/libharness.o \
	../jamvm/alloc-stub.o \
	../jamvm/resolve-stub.o \
	../arch-mmix/instruction.o \
	../arch-mmix/use-def.o \
	arithmetic-bc-test.o \
	basic-block-test.o \
	bc-test-utils.o \
	branch-bc-test.o \
	bytecode-to-ir-test.o \
	cfg-analyzer-test.o \
	compilation-unit-test.o \
	expression-test.o \
	interval-test.o \
	invoke-bc-test.o \
	linear-scan-test.o \
	liveness-test.o \
	live-range-test.o \
	load-store-bc-test.o \
	object-bc-test.o \
	ostack-bc-test.o \
	stack-slot-test.o \
	spill-reload-test.o \
	tree-printer-test.o \
	typeconv-bc-test.o \

quiet_cmd_run_tests = RUNTEST $(empty) $(RUNNER)
      cmd_run_tests = ./$(RUNNER)

quiet_cmd_cc_test_runner = MAKE $(empty)   $(empty) $(RUNNER)
      cmd_cc_test_runner = $(CC) $(CCFLAGS) $(INCLUDE) *.o $(SUITE) -o $(RUNNER) $(LIBS)

test: $(SUITE) $(OBJS)
	$(call cmd,cc_test_runner)
	$(call cmd,run_tests)

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
