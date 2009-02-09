CCFLAGS	?= -rdynamic -g -Wall -Wundef -Wsign-compare -Os -std=gnu99
INCLUDE	?= -I../include/ -I. -I../libharness -I../../include -I../../jit/glib
LIBS	?= -lpthread -lm -ldl -lz -lbfd -lopcodes -liberty

include ../../scripts/Makefile.include

quiet_cmd_cc_o_c = CC $(empty)     $(empty) $@
      cmd_cc_o_c = $(CC) $(CCFLAGS) $(INCLUDE) $(DEFINES) -c $< -o `basename $@`

%.o: %.c
	$(call cmd,cc_o_c)

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
