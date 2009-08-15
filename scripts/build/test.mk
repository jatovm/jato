ARCH_CONFIG=../../arch/$(ARCH)/include/arch/config$(ARCH_POSTFIX).h

DEFAULT_CFLAGS	?= -rdynamic -g -Wall -Wundef -Wsign-compare -Os -std=gnu99 -D_GNU_SOURCE
# XXX: Temporary hack -Vegard
DEFAULT_CFLAGS	+= -DNOT_IMPLEMENTED='fprintf(stderr, "%s:%d: warning: %s not implemented\n", __FILE__, __LINE__, __func__)'

INCLUDE		?= -I../include/ -I. -I../libharness -I../../include \
		   -I../../jit/glib -I../../cafebabe/include/ \
		   -I../../arch/$(ARCH)/include -include $(ARCH_CONFIG)
DEFAULT_LIBS	?= -lrt -lpthread -lm -ldl -lz -lbfd -lopcodes -liberty

ifdef TESTS
	TESTS_C = $(TESTS:.o=.c)
else
	TESTS_C = '*.c'
endif

TOPLEVEL_C_SRCS := $(wildcard $(patsubst %.o,../../%.c,$(TOPLEVEL_OBJS)))
TOPLEVEL_C_OBJS := $(patsubst ../../%.c,toplevel/%.o,$(TOPLEVEL_C_SRCS))

TOPLEVEL_S_SRCS := $(wildcard $(patsubst %.o,../../%.S,$(TOPLEVEL_OBJS)))
TOPLEVEL_S_OBJS := $(patsubst ../../%.S,toplevel/%.o,$(TOPLEVEL_S_SRCS))

$(TOPLEVEL_C_OBJS): toplevel/%.o: ../../%.c
	@mkdir -p $(dir $@)
	$(E) "  CC      " $@
	$(Q) $(CC) $(ARCH_CFLAGS) $(DEFAULT_CFLAGS) $(CFLAGS) $(INCLUDE) $(DEFINES) -c $< -o $@

$(TOPLEVEL_S_OBJS): toplevel/%.o: ../../%.S
	@mkdir -p $(dir $@)
	$(E) "  AS      " $@
	$(Q) $(CC) $(ARCH_CFLAGS) $(DEFAULT_CFLAGS) $(CFLAGS) $(INCLUDE) $(DEFINES) -c $< -o $@

TEST_C_SRCS := $(wildcard $(patsubst %.o,%.c,$(TEST_OBJS)))
TEST_C_OBJS := $(patsubst %.c,%.o,$(TEST_C_SRCS))

TEST_S_SRCS := $(wildcard $(patsubst %.o,%.S,$(TEST_OBJS)))
TEST_S_OBJS := $(patsubst %.S,%.o,$(TEST_S_SRCS))

$(TEST_C_OBJS): %.o: %.c
	$(E) "  CC      " $@
	$(Q) $(CC) $(ARCH_CFLAGS) $(DEFAULT_CFLAGS) $(CFLAGS) $(INCLUDE) $(DEFINES) -c $< -o $@

$(TEST_S_OBJS): %.o: %.S
	$(E) "  AS      " $@
	$(Q) $(CC) $(ARCH_CFLAGS) $(DEFAULT_CFLAGS) $(CFLAGS) $(INCLUDE) $(DEFINES) -c $< -o $@

test: run

ALL_OBJS := $(TOPLEVEL_C_OBJS) $(TOPLEVEL_S_OBJS) $(TEST_C_OBJS) $(TEST_S_OBJS)

$(RUNNER): $(SUITE) $(ALL_OBJS)
	$(E) "  LD      " $@
	$(Q) $(CC) $(ARCH_CFLAGS) $(DEFAULT_CFLAGS) $(CFLAGS) $(INCLUDE) $(ALL_OBJS) $(SUITE) -o $(RUNNER) $(LIBS) $(DEFAULT_LIBS)

run: $(RUNNER)
	$(E) "  RUN     " $@
	$(Q) ./$(RUNNER)

$(SUITE): FORCE
	$(E) "  SUITE   " $@
	$(Q) sh ../../scripts/build/make-tests.sh $(TESTS_C) > $@

clean: FORCE
	$(E) "  CLEAN"
	$(Q) rm -f *.o $(RUNNER) $(SUITE) $(ALL_OBJS)

PHONY += FORCE
FORCE:
# DO NOT DELETE
