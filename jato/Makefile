prefix = /usr/local
with_classpath_install_dir = /usr/local/classpath

EXECUTABLE = jato
CC = gcc
CCFLAGS = -rdynamic -g -Wall -Wundef -Wsign-compare -Os -std=gnu99
DEFINES = -DINSTALL_DIR=\"$(prefix)\" -DCLASSPATH_INSTALL_DIR=\"$(with_classpath_install_dir)\"
INCLUDE = -I. -I../src -I./glib -I../include -I../jit -I../jit/glib
LIBS = -lpthread -lm -ldl -lz
OBJS = jato.o ../src/access.o ../src/alloc.o ../src/cast.o ../src/class.o \
	../src/direct.o ../src/dll.o ../src/excep.o ../src/execute.o \
	../src/hash.o ../src/interp.o ../src/jni.o ../src/lock.o \
	../src/natives.o ../src/properties.o ../src/reflect.o \
	../src/resolve.o ../src/string.o ../src/thread.o ../src/utf8.o \
	../src/zip.o ../src/dll_ffi.o ../src/frame.o \
	../src/os/linux/dll_os.o ../src/os/linux/i386/dll_md.o \
	../src/os/linux/i386/init.o ../jit/jit.o ../jit/compilation-unit.o \
	../jit/stack.o ../jit/basic-block.o ../jit/statement.o \
	../jit/instruction.o ../jit/expression.o ../jit/jit-compiler.o \
	../jit/x86-objcode.o ../jit/alloc.o ../jit/bytecode-to-ir.o \
	../jit/bytecodes.o ../jit/bytecode-cfg-builder.o ../vm/bitmap.o \
	../jit/insn-selector.o ../jit/arithmetic-bc.o ../jit/load-store-bc.o \
	../vm/backtrace.o ../jit/ostack-bc.o ../jit/typeconv-bc.o

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

# If quiet is set, only print short version of command
cmd = @$(if $($(quiet)cmd_$(1)),\
      echo '  $($(quiet)cmd_$(1))' &&) $(cmd_$(1))

quiet_cmd_cc_o_c = CC $(empty)     $(empty) $@
      cmd_cc_o_c = $(CC) $(CCFLAGS) $(INCLUDE) $(DEFINES) -c $< -o $@

%.o: %.c
	$(call cmd,cc_o_c)

all: $(EXECUTABLE)

ARCH_H = ../include/vm/arch.h

quiet_cmd_ln_arch_h = LN $(empty)     $(empty) $@
      cmd_ln_arch_h = ln -fsn ../../src/arch/i386.h $@

$(ARCH_H):
	$(call cmd,ln_arch_h)

$(OBJS): $(ARCH_H)

quiet_cmd_cc_exec = LD $(empty)     $(empty) $(EXECUTABLE)
      cmd_cc_exec = $(CC) $(CCFLAGS) $(INCLUDE) $(DEFINES) $(LIBS) $(OBJS) -o $(EXECUTABLE)

$(EXECUTABLE): $(OBJS)
	$(call cmd,cc_exec)

quiet_cmd_clean = CLEAN
      cmd_clean = rm -rf $(OBJS) $(EXECUTABLE)

clean:
	$(call cmd,clean)
