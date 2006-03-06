CC = gcc
CCFLAGS = -rdynamic -g -Wall -Wundef -Werror -Os -std=gnu99 \
	-DYYENABLE_NLS=0 -DYYLTYPE_IS_TRIVIAL=0
INCLUDE = -I. -I.. -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include
LIBS = -lglib-2.0
OBJS = monoburg.o parser.o

%.o: %.c
	$(CC) $(CCFLAGS) $(INCLUDE) -c $< -o $@

all: $(OBJS)
	$(CC) $(CCFLAGS) $(OBJS) $(LIBS) -o monoburg

$(OBJS):

parser.c: monoburg.y
	bison monoburg.y -o parser.c

clean:
	rm -f parser.c monoburg *.o
