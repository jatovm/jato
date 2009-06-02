#ifndef _VM_GUARD_PAGE_
#define _VM_GUARD_PAGE_

void *alloc_guard_page(void);
int hide_guard_page(void *page_ptr);
int unhide_guard_page(void *page_ptr);

#endif /* _VM_GUARD_PAGE_ */
