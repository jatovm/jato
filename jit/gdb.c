/*
 * GDB support for proper backtraces.
 *
 * Copyright (C) 2010 Eduard - Gabriel Munteanu
 *
 * This file is released under the GPL version 2 with the following
 * clarification and special exception:
 *
 *     Linking this library statically or dynamically with other modules is
 *     making a combined work based on this library. Thus, the terms and
 *     conditions of the GNU General Public License cover the whole
 *     combination.
 *
 *     As a special exception, the copyright holders of this library give you
 *     permission to link this library with independent modules to produce an
 *     executable, regardless of the license terms of these independent
 *     modules, and to copy and distribute the resulting executable under terms
 *     of your choice, provided that you also meet, for each linked independent
 *     module, the terms and conditions of the license of that module. An
 *     independent module is a module which is not derived from or based on
 *     this library. If you modify this library, you may extend this exception
 *     to your version of the library, but you are not obligated to do so. If
 *     you do not wish to do so, delete this exception statement from your
 *     version.
 *
 * Please refer to the file LICENSE for details.
 */

#ifdef CONFIG_GDB

#include "jit/compilation-unit.h"
#include "jit/elf.h"
#include "jit/gdb.h"

#include "vm/method.h"
#include "vm/class.h"

#include <ctype.h>
#include <string.h>
#include <pthread.h>

/*
 * The following are taken from the
 * GDB manual, section "29 JIT Compilation Interface".
 */

typedef enum
{
	JIT_NOACTION = 0,
	JIT_REGISTER_FN,
	JIT_UNREGISTER_FN
} jit_actions_t;

struct jit_code_entry
{
	struct jit_code_entry *next_entry;
	struct jit_code_entry *prev_entry;
	const char *symfile_addr;
	uint64_t symfile_size;
};

struct jit_descriptor
{
	uint32_t version;
        /*
	 * This type should be jit_actions_t, but we use uint32_t
	 * to be explicit about the bitwidth.
	 */
        uint32_t action_flag;
	struct jit_code_entry *relevant_entry;
        struct jit_code_entry *first_entry;
};

/* GDB puts a breakpoint in this function.  */
static void __attribute__ ((noinline)) __jit_debug_register_code(void) { };

/*
 * Make sure to specify the version statically, because the
 * debugger may check the version before we can set it.
 */
static struct jit_descriptor __jit_debug_descriptor = { 1, 0, 0, 0 };

/* ----------------------------------------------------------------------- */

static struct jit_code_entry ent;
static pthread_mutex_t gdb_mutex = PTHREAD_MUTEX_INITIALIZER;

static void gdb_enable(void)
{
	__jit_debug_descriptor.action_flag = JIT_REGISTER_FN;
	__jit_debug_register_code();
}

static void gdb_disable(void)
{
	__jit_debug_descriptor.action_flag = JIT_UNREGISTER_FN;
	__jit_debug_register_code();
}

void gdb_init(void)
{
	struct jit_descriptor *desc = &__jit_debug_descriptor;

	ent.symfile_addr = elf_init();
	ent.symfile_size = elf_get_size();

	/*
	 * We use a single entry which points to the
	 * entire code and the associated ELF structures.
	 */
	desc->first_entry = desc->relevant_entry = &ent;
	gdb_enable();
}

static void gdb_mangle(char *name)
{
	char *p;

	/* FIXME: Implement proper mangling. */
	for (p = name; *p; p++)
		if (!isalnum(*p))
			*p = '_';
}

static void gdb_register_code(char *name, struct buffer *buf)
{
	struct jit_descriptor *desc = &__jit_debug_descriptor;

	assert(desc->first_entry);

	gdb_mangle(name);

	pthread_mutex_lock(&gdb_mutex);
	gdb_disable();
	elf_add_symbol(name, buffer_ptr(buf), buffer_offset(buf));
	desc->first_entry->symfile_size = elf_get_size();
	gdb_enable();
	pthread_mutex_unlock(&gdb_mutex);
}

void gdb_register_method(struct vm_method *method)
{
	struct compilation_unit *cu = method->compilation_unit;
	char name[256], *class, *vmm;

	class = method->class->name;
	vmm = method->name;

	assert(strlen(class) + 1 + strlen(vmm) < 256);

	sprintf(name, "%s.%s", class, vmm);

	gdb_register_code(name, cu->objcode);
}

void gdb_register_trampoline(struct vm_method *method)
{
	struct jit_trampoline *trampoline = method->trampoline;
	char name[256], *class, *vmm;

	class = method->class->name;
	vmm = method->name;

	assert(strlen(class) + 1 + strlen(vmm) + 3 < 256);

	sprintf(name, "%s.%s.TP", class, vmm);

	gdb_register_code(name, trampoline->objcode);
}

#endif
