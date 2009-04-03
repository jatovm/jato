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

INCLUDES	= -I. -I..
CFLAGS		+= $(INCLUDES)

CFLAGS		+= $(shell pkg-config --cflags glib-2.0)
LIBS		+= $(shell pkg-config --libs glib-2.0)

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
	$(Q) $(CC) $(CFLAGS) $(OBJS) -o $@ $(LIBS)

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
