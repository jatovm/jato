uname_S	:= $(shell sh -c 'uname -s 2>/dev/null || echo not')
uname_R	:= $(shell sh -c 'uname -r 2>/dev/null || echo not')

# External programs
CC	:= gcc

# Set up source directory for GNU Make
srcdir		:= $(CURDIR)
VPATH		:= $(srcdir)

EXTRA_WARNINGS := -Wcast-align
EXTRA_WARNINGS += -Wformat
EXTRA_WARNINGS += -Wformat-security
EXTRA_WARNINGS += -Wformat-y2k
EXTRA_WARNINGS += -Wshadow
EXTRA_WARNINGS += -Winit-self
EXTRA_WARNINGS += -Wpacked
EXTRA_WARNINGS += -Wredundant-decls
EXTRA_WARNINGS += -Wstrict-aliasing=3
EXTRA_WARNINGS += -Wswitch-default
EXTRA_WARNINGS += -Wswitch-enum
EXTRA_WARNINGS += -Wno-system-headers
EXTRA_WARNINGS += -Wundef
EXTRA_WARNINGS += -Wwrite-strings
EXTRA_WARNINGS += -Wbad-function-cast
EXTRA_WARNINGS += -Wmissing-declarations
EXTRA_WARNINGS += -Wmissing-prototypes
EXTRA_WARNINGS += -Wnested-externs
EXTRA_WARNINGS += -Wold-style-definition
EXTRA_WARNINGS += -Wstrict-prototypes
EXTRA_WARNINGS += -Wdeclaration-after-statement

# Compile flags
CFLAGS	:= -I$(srcdir)/include -Wall $(EXTRA_WARNINGS) -g -O6 -std=gnu99

# Output to current directory by default
O =

# Make the build silent by default
V =
ifeq ($(strip $(V)),)
	E = @echo
	Q = @
else
	E = @\#
	Q =
endif
export E Q

# Project files
PROGRAM := gen

OBJS += gen.o

DEPS		:= $(patsubst %.o,%.d,$(OBJS))

# Targets
all: sub-make
.DEFAULT: all
.PHONY: all

ifneq ($(O),)
sub-make: $(O) $(FORCE)
	$(Q) $(MAKE) --no-print-directory -C $(O) -f ../Makefile srcdir=$(CURDIR) _all
else
sub-make: _all
endif

_all: $(PROGRAM) $(LIB_FILE)
.PHONY: _all

$(O):
ifneq ($(O),)
	$(E) "  MKDIR   " $@
	$(Q) mkdir -p $(O)
endif

%.d: %.c
	$(Q) $(CC) -M -MT $(patsubst %.d,%.o,$@) $(CFLAGS) $< -o $@

%.o: %.c
	$(E) "  CC      " $@
	$(Q) $(CC) -c $(CFLAGS) $< -o $@

$(PROGRAM): $(DEPS) $(LIB_FILE) $(OBJS)
	$(E) "  LINK    " $@
	$(Q) $(CC) $(OBJS) $(LIBS) -o $(PROGRAM)

clean:
	$(E) "  CLEAN"
	$(Q) rm -f $(PROGRAM) $(OBJS) $(DEPS)
.PHONY: clean

PHONY += FORCE

FORCE:

# Deps
-include $(DEPS)
