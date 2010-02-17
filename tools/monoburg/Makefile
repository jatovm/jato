V =

PROGRAM = monoburg

OBJS = \
	monoburg.o	\
	parser.o

CC	?= gcc
LINK	?= $(CC)

DEFAULT_CFLAGS	+= -Wall -g -rdynamic -std=gnu99

WARNINGS	= -Wundef
DEFAULT_CFLAGS	+= $(WARNINGS)

OPTIMIZATIONS	= -Os
DEFAULT_CFLAGS	+= $(OPTIMIZATIONS)

DEFINES		= -DYYENABLE_NLS=0 -DYYLTYPE_IS_TRIVIAL=0
DEFAULT_CFLAGS	+= $(DEFINES)

INCLUDES	= -I. -I..
DEFAULT_CFLAGS	+= $(INCLUDES)

DEFAULT_CFLAGS	+= $(shell pkg-config --cflags glib-2.0)
DEFAULT_LIBS	+= $(shell pkg-config --libs glib-2.0)

ifeq ($(strip $(V)),)
	E = @echo
	Q = @
else
	E = @\#
	Q =
endif
export E Q

all: $(PROGRAM)
.PHONY: all
.DEFAULT: all

%.o: %.c
	$(E) "  CC      " $@
	$(Q) $(CC) -c $(DEFAULT_CFLAGS) $(CFLAGS) $< -o $@
	$(Q) $(CC) -MM $(DEFAULT_CFLAGS) $(CFLAGS) -MT $@ $*.c -o $*.d

$(OBJS):

$(PROGRAM): $(OBJS)
	$(E) "  LINK    " $@
	$(Q) $(LINK) $(DEFAULT_CFLAGS) $(CFLAGS) $(OBJS) -o $(PROGRAM) $(LIBS) $(DEFAULT_LIBS)


%.o: %.c
	$(E) "  CC      " $@
	$(Q) $(CC) -c $(DEFAULT_CFLAGS) $(CFLAGS) $< -o $@

parser.c: monoburg.y
	$(E) "  BISON   " $@
	$(Q) bison monoburg.y -o parser.c

clean:
	$(E) "  CLEAN"
	$(Q) - rm -f parser.c monoburg *.o
.PHONY: all
