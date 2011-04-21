#include <stdio.h>
#include "vm/class.h"
#include "vm/object.h"
#include "vm/method.h"
#include "vm/debug.h"

void debug_print_vm_method(struct vm_method *method)
{
	if (method) {
		printf("  method : %s %s(", method->type, method->name);
		for (int i = 0; i < method->args_count; i++ ) {
			printf(".");
		}
		printf(")\n");
	}
	else {
		printf("Unable to print null vm_method!\n");
	}
}

void debug_print_vm_class(struct vm_class *class)
{
	if (class) {
		printf("vm_class address : %p\n", class);
		if (class->name) printf("class->name : %s\n", class->name);
		printf("class->super->name : %s\n", class->super->name);
		printf("class->kind : %d\n", class->kind);
		printf("class->state : %d\n", class->state);
		if (class->declaring_class) printf("class->declaring_class->name : %s\n", class->declaring_class->name);
		if (class->enclosing_class) printf("class->enclosing_class->name : %s\n", class->enclosing_class->name);

		for(uint i = 0; i < class->nr_methods; i++) {
			debug_print_vm_method(&class->methods[i]);
		}
	}
	else {
		printf("Unable to print null vm_class!\n");
	}
}

void debug_print_vm_object(struct vm_object *obj)
{
	if (obj) {
		printf("vm_object address : %p\n", obj);
		debug_print_vm_class(obj->class);
	}
	else {
		printf("Unable to print null vm_object!\n");
	}
}
