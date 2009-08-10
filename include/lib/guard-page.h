#ifndef _LIB_GUARD_PAGE_
#define _LIB_GUARD_PAGE_

void *alloc_guard_page(void);
void *alloc_offset_guard(unsigned long valid_size, unsigned long overflow_size);
int hide_guard_page(void *page_ptr);
int unhide_guard_page(void *page_ptr);

#endif /* _LIB_GUARD_PAGE_ */
