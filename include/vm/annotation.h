#ifndef JATO__VM_ANNOTATION_H
#define JATO__VM_ANNOTATION_H

struct cafebabe_annotation;
struct vm_class;

struct vm_element_value_pair {
	char				*name;
	struct vm_object		*value;
};

struct vm_annotation {
	char				*type;
	unsigned long			nr_elements;
	struct vm_element_value_pair	*elements;
};

struct vm_object *vm_annotation_to_object(struct vm_annotation *vma);
struct vm_annotation *vm_annotation_parse(struct vm_class *vmc, struct cafebabe_annotation *annotation);
void vm_annotation_free(struct vm_annotation *vma);

#endif /* JATO__VM_ANNOTATION_H */
