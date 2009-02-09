CFLAGS	?= -rdynamic -g -Wall -Wundef -Wsign-compare -Os -std=gnu99
INCLUDE	?= -I../include/ -I. -I../libharness -I../../include -I../../jit/glib
LIBS	?= -lpthread -lm -ldl -lz -lbfd -lopcodes -liberty

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
	$(Q) sh ../../scripts/build/make-tests.sh *.c > $@

clean: FORCE
	$(E) "  CLEAN"
	$(Q) rm -f *.o $(RUNNER) $(SUITE)

PHONY += FORCE
FORCE:
# DO NOT DELETE
