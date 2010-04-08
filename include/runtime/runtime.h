#ifndef RUNTIME_RUNTIME_H
#define RUNTIME_RUNTIME_H

struct vm_object;

void native_vmruntime_gc(void);
void native_vmruntime_exit(int status);
void native_vmruntime_run_finalization_for_exit(void);
struct vm_object *native_vmruntime_maplibraryname(struct vm_object *name);
int native_vmruntime_native_load(struct vm_object *name,
				 struct vm_object *classloader);

#endif /* RUNTIME_RUNTIME_H */
