ARCH_CONFIG=../../arch/$(ARCH)/include/arch/config$(ARCH_POSTFIX).h

DEFAULT_CFLAGS	?= -rdynamic -g -Wall -Wundef -Wsign-compare -Os -std=gnu99 -D_GNU_SOURCE
# XXX: Temporary hack -Vegard
DEFAULT_CFLAGS	+= -DNOT_IMPLEMENTED='fprintf(stderr, "%s:%d: warning: %s not implemented\n", __FILE__, __LINE__, __func__)'

INCLUDE		?= -I../include/ -I. -I../libharness -I../../include -I../../jit/glib -I../../cafebabe/include/ -include $(ARCH_CONFIG)
DEFAULT_LIBS	?= -lpthread -lm -ldl -lz -lbfd -lopcodes -liberty

ifdef TESTS
	TESTS_C = $(TESTS:.o=.c)
else
	TESTS_C = '*.c'
endif

%.o: %.c
	$(E) "  CC      " $@
	$(Q) $(CC) $(ARCH_CFLAGS) $(DEFAULT_CFLAGS) $(CFLAGS) $(INCLUDE) $(DEFINES) -c $< -o `echo $@ | sed "s\/\-\g" | sed "s\[.][.]\p\g"`

%.o: %.S
	$(E) "  AS      " $@
	$(Q) $(AS) $(AFLAGS) $< -o `echo $@ | sed "s\/\-\g" | sed "s\[.][.]\p\g"`

test: $(RUNNER)

$(RUNNER): $(SUITE) $(OBJS)
	$(E) "  LD      " $@
	$(Q) $(CC) $(ARCH_CFLAGS) $(DEFAULT_CFLAGS) $(CFLAGS) $(INCLUDE) *.o $(SUITE) -o $(RUNNER) $(LIBS) $(DEFAULT_LIBS)
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
