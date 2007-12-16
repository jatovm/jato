V =

PROGRAM = monoburg

OBJS = \
	monoburg.o	\
	parser.o

CC = gcc

CFLAGS		+= -Wall -g -rdynamic -Werror -std=gnu99

WARNINGS	= -Wundef
CFLAGS		+= $(WARNINGS)

OPTIMIZATIONS	= -Os
CFLAGS		+= $(OPTIMIZATIONS)

DEFINES		= -DYYENABLE_NLS=0 -DYYLTYPE_IS_TRIVIAL=0
CFLAGS		+= $(DEFINES)

INCLUDES	= -I. -I.. -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include
CFLAGS		+= $(INCLUDES)

LIBS		= -lglib-2.0

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

$(OBJS):

$(PROGRAM): $(OBJS)
	$(E) "  CC      " $@
	$(Q) $(CC) $(CFLAGS) $(OBJS) $(LIBS) -o $@

%.o: %.c
	$(E) "  CC      " $@
	$(Q) $(CC) -c $(CFLAGS) $< -o $@

parser.c: monoburg.y
	$(E) "  BISON   " $@
	$(Q) bison monoburg.y -o parser.c

clean:
	$(E) "  CLEAN"
	$(Q) - rm -f parser.c monoburg *.o
.PHONY: all
