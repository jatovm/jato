MONOBURG=./monoburg/monoburg
CC = gcc
CCFLAGS = -rdynamic -g -Wall -Wundef -Wsign-compare -Os -std=gnu99
DEFINES = -DINSTALL_DIR=\"$(prefix)\" -DCLASSPATH_INSTALL_DIR=\"$(with_classpath_install_dir)\"
INCLUDE = -I. -I../src -I./glib -I../include -I../jit -I../jit/glib
LIBS = -lpthread -lm -ldl -lz -lbfd -lopcodes

ARCH_H = include/vm/arch.h
EXECUTABLE = jato-exe

# GNU classpath installation path
prefix = /usr/local
with_classpath_install_dir = /usr/local/classpath

# Default to quiet output and add verbose printing only if V=1 is passed
# as parameter to make.
ifdef V
  ifeq ("$(origin V)", "command line")
    BUILD_VERBOSE = $(V)
  endif
endif

ifndef BUILD_VERBOSE
  BUILD_VERBOSE = 0
endif

ifeq ($(BUILD_VERBOSE), 1)
  quiet =
else
  quiet = quiet_
endif

OBJS =  \
	jato/jato.o \
	jit/alloc.o \
	jit/arithmetic-bc.o \
	jit/basic-block.o \
	jit/branch-bc.o \
	jit/bytecodes.o \
	jit/bytecode-to-ir.o \
	jit/cfg-analyzer.o \
	jit/compilation-unit.o \
	jit/disass-common.o \
	jit/disass.o \
	jit/expression.o \
	jit/insn-selector.o \
	jit/instruction.o \
	jit/invoke-bc.o \
	jit/jit-compiler.o \
	jit/jvm_types.o \
	jit/load-store-bc.o \
	jit/object-bc.o \
	jit/ostack-bc.o \
	jit/statement.o \
	jit/tree-printer.o \
	jit/typeconv-bc.o \
	jit/x86-frame.o \
	jit/x86-objcode.o \
	src/access.o \
	src/alloc.o \
	src/cast.o \
	src/class.o \
	src/direct.o \
	src/dll_ffi.o \
	src/dll.o \
	src/excep.o \
	src/execute.o \
	src/frame.o \
	src/hash.o \
	src/interp.o \
	src/jni.o \
	src/lock.o \
	src/natives.o \
	src/os/linux/i386/dll_md.o \
	src/os/linux/i386/init.o \
	src/os/linux/os.o \
	src/properties.o \
	src/reflect.o \
	src/resolve.o \
	src/string.o \
	src/thread.o \
	src/utf8.o \
	src/zip.o \
	vm/backtrace.o \
	vm/bitmap.o \
	vm/buffer.o \
	vm/debug-dump.o \
	vm/natives.o \
	vm/stack.o \
	vm/string.o
	
# If quiet is set, only print short version of command
cmd = @$(if $($(quiet)cmd_$(1)),\
      echo '  $($(quiet)cmd_$(1))' &&) $(cmd_$(1))

all: $(EXECUTABLE)

compile: gen
	$(MAKE) -f scripts/Makefile.compile obj=jit/
	$(MAKE) -f scripts/Makefile.compile obj=src/
	$(MAKE) -f scripts/Makefile.compile obj=vm/
	$(MAKE) -f scripts/Makefile.compile obj=jato/

quiet_cmd_ln_arch_h = LN $(empty)     $(empty) $@
      cmd_ln_arch_h = ln -fsn ../../src/arch/i386.h $@

$(ARCH_H):
	$(call cmd,ln_arch_h)

gen:
	$(MONOBURG) -p -e jit/insn-selector.brg > jit/insn-selector.c

clean:
	$(MAKE) -f scripts/Makefile.clean obj=jit/
	$(MAKE) -f scripts/Makefile.clean obj=src/
	$(MAKE) -f scripts/Makefile.clean obj=vm/
	$(MAKE) -f scripts/Makefile.clean obj=jato/
	rm -f jit/insn-selector.c
	rm -f $(EXECUTABLE)
	rm -f $(ARCH_H)

quiet_cmd_cc_exec = LD $(empty)     $(empty) $(EXECUTABLE)
      cmd_cc_exec = $(CC) $(CCFLAGS) $(INCLUDE) $(DEFINES) $(LIBS) $(OBJS) -o $(EXECUTABLE)

$(EXECUTABLE): $(ARCH_H) compile
	$(call cmd,cc_exec)
