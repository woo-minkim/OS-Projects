#ifndef FILE_H
#define FILE_H

#include <threads/palloc.h>
#include "page.h"

void lru_init(void);
struct page *alloc_page(enum palloc_flags flags);
void evict_page();
void release_page(void *page_kernel_addr);

#endif
