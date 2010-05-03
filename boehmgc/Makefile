AR		:= ar

#
# Set Boehm GC configuration variables.
#
DEFAULT_CFLAGS		+= -DSILENT=1 -DGC_USE_LD_WRAP -D_REENTRANT -DGC_LINUX_THREADS -lpthreads -Iinclude

BOEHMGC_OBJS	+= allchblk.o
BOEHMGC_OBJS	+= alloc.o
BOEHMGC_OBJS	+= blacklst.o
BOEHMGC_OBJS	+= dyn_load.o
BOEHMGC_OBJS	+= finalize.o
BOEHMGC_OBJS	+= headers.o
BOEHMGC_OBJS	+= mach_dep.o
BOEHMGC_OBJS	+= malloc.o
BOEHMGC_OBJS	+= mallocx.o
BOEHMGC_OBJS	+= mark.o
BOEHMGC_OBJS	+= mark_rts.o
BOEHMGC_OBJS	+= misc.o
BOEHMGC_OBJS	+= new_hblk.o
BOEHMGC_OBJS	+= obj_map.o
BOEHMGC_OBJS	+= os_dep.o
BOEHMGC_OBJS	+= pthread_stop_world.o
BOEHMGC_OBJS	+= pthread_support.o
BOEHMGC_OBJS	+= reclaim.o
BOEHMGC_OBJS	+= stubborn.o

BOEHMGC_LIB	= libboehmgc.a

%.o: %.c
	$(E) "  CC      " $@
	$(Q) $(CC) -c $(DEFAULT_CFLAGS) $(CFLAGS) $< -o $@

$(BOEHMGC_LIB): $(BOEHMGC_OBJS)
	$(E) "  AR      " $@
	$(Q) rm -f $@ && $(AR) rcs $@ $(BOEHMGC_OBJS)

clean:
	$(E) "  CLEAN"
	$(Q) - rm -f $(BOEHMGC_OBJS)
	$(Q) - rm -f $(BOEHMGC_LIB)
.PHONY: clean

include ../scripts/build/common.mk
