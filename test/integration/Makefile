uname_S	:= $(shell sh -c 'uname -s 2>/dev/null || echo not')
uname_R	:= $(shell sh -c 'uname -r 2>/dev/null || echo not')

# External programs
CC	:= gcc
AR	:= ar

# Set up source directory for GNU Make
srcdir		:= $(CURDIR)
VPATH		:= $(srcdir)

uname_M		:= $(shell uname -m | sed -e s/i.86/i386/ | sed -e s/i86pc/i386/)
ARCH		:= $(shell sh ../../scripts/gcc-arch.sh $(CC))
SYS		:= $(shell uname -s | tr A-Z a-z)

include ../../scripts/build/arch.mk

EXTRA_WARNINGS := -Wcast-align
EXTRA_WARNINGS += -Wformat
EXTRA_WARNINGS += -Wformat-security
EXTRA_WARNINGS += -Wformat-y2k
#EXTRA_WARNINGS += -Wshadow
EXTRA_WARNINGS += -Winit-self
#EXTRA_WARNINGS += -Wpacked
EXTRA_WARNINGS += -Wredundant-decls
EXTRA_WARNINGS += -Wstrict-aliasing=3
#EXTRA_WARNINGS += -Wswitch-default
#EXTRA_WARNINGS += -Wswitch-enum
EXTRA_WARNINGS += -Wno-system-headers
#EXTRA_WARNINGS += -Wundef
#EXTRA_WARNINGS += -Wwrite-strings
#EXTRA_WARNINGS += -Wbad-function-cast
EXTRA_WARNINGS += -Wmissing-declarations
EXTRA_WARNINGS += -Wmissing-prototypes
EXTRA_WARNINGS += -Wnested-externs
EXTRA_WARNINGS += -Wold-style-definition
EXTRA_WARNINGS += -Wstrict-prototypes
EXTRA_WARNINGS += -Wdeclaration-after-statement

LLVM_CFLAGS     := $(shell llvm-config --cflags | sed -e "s/-DNDEBUG//g")
LLVM_LIBS       := $(shell llvm-config --libs)
LLVM_LDFLAGS    := $(shell llvm-config --ldflags)

# Compile flags
CFLAGS		:= -I$(srcdir)/include -I../../include -I../../arch/$(ARCH)/include -Wall $(EXTRA_WARNINGS) -g -O3 -std=gnu99
CFLAGS		+= $(LLVM_CFLAGS)

# Linker flags
LDFLAGS		:= -Wl,--wrap -Wl,pthread_create -Wl,--wrap -Wl,pthread_join \
             -Wl,--wrap -Wl,pthread_detach -Wl,--wrap -Wl,pthread_sigmask \
             -Wl,--wrap -Wl,sleep
LDFLAGS		+= $(LLVM_CFLAGS)

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
PROGRAM := bytecode-test

DEFINES =
CONFIG_OPTS =
COMPAT_OBJS =

ifeq ($(uname_S),Darwin)
	CONFIG_OPTS += -DCONFIG_NEED_STRNDUP=1
	COMPAT_OBJS += compat/strndup.o

	CONFIG_OPTS += -DCONFIG_NEED_POSIX_FALLOCATE=1
	CONFIG_OPTS += -DCONFIG_NEED_POSIX_FADVISE=1
endif
ifeq ($(uname_S),Linux)
	DEFINES += -D_FILE_OFFSET_BITS=64
	DEFINES += -D_GNU_SOURCE
endif
ifeq ($(uname_S),SunOS)
	DEFINES += -D_FILE_OFFSET_BITS=64

	CONFIG_OPTS += -DCONFIG_NEED_STRNDUP=1
	COMPAT_OBJS += compat/strndup.o
endif

OBJS += bytecode-test.o

OBJS += $(COMPAT_OBJS)

LIB_FILE := ../../libjvm.a

LIBS := -L../../ -ljvm -lrt -lpthread -lm -ldl -lz -lzip -lbfd -lopcodes -liberty -L../../boehmgc -lboehmgc

LIBS += $(LLVM_LIBS)

CFLAGS += $(DEFINES)
CFLAGS += $(CONFIG_OPTS)

CFLAGS += -include../../arch/$(ARCH)/include/arch/config$(ARCH_POSTFIX).h

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

_all: $(PROGRAM)
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
	$(Q) $(CXX) $(LDFLAGS) $(OBJS) $(LIBS) -o $(PROGRAM)

check: $(PROGRAM)
	$(E) "  CHECK"
	$(Q) ./$(PROGRAM)
.PHONY: check

clean:
	$(E) "  CLEAN"
	$(Q) rm -f $(PROGRAM) $(OBJS) $(DEPS)
.PHONY: clean

tags: FORCE
	$(E) "  TAGS"
	$(Q) rm -f tags
	$(Q) ctags-exuberant -a *.c
	$(Q) ctags-exuberant -a -R include

PHONY += FORCE

FORCE:

# Deps
-include $(DEPS)
