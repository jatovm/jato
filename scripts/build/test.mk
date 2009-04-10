ARCH_CONFIG=../../arch/$(ARCH)/include/arch/config$(ARCH_POSTFIX).h

CFLAGS	?= -rdynamic -g -Wall -Wundef -Wsign-compare -Os -std=gnu99
INCLUDE	?= -I../include/ -I. -I../libharness -I../../include -I../../jit/glib -include $(ARCH_CONFIG)
LIBS	?= -lpthread -lm -ldl -lz -lbfd -lopcodes -liberty

ifdef TESTS
	TESTS_C = $(TESTS:.o=.c)
else
	TESTS_C = '*.c'
endif

%.o: %.c
	$(E) "  CC      " $@
	$(Q) $(CC) $(CFLAGS) $(INCLUDE) $(DEFINES) -c $< -o `basename $@`

test: $(RUNNER)

$(RUNNER): $(SUITE) $(OBJS)
	$(E) "  LD      " $@
	$(Q) $(CC) $(CFLAGS) $(INCLUDE) *.o $(SUITE) -o $(RUNNER) $(LIBS)
	$(E) "  RUN     " $@
	$(Q) ./$(RUNNER)

$(OBJS): FORCE

$(SUITE): FORCE
	$(E) "  SUITE   " $@
	$(Q) sh ../../scripts/build/make-tests.sh $(TESTS_C) > $@

clean: FORCE
	$(E) "  CLEAN"
	$(Q) rm -f *.o $(RUNNER) $(SUITE)

PHONY += FORCE
FORCE:
# DO NOT DELETE
