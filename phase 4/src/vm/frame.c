#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "userprog/syscall.h"
#include "userprog/pagedir.h"
#include <threads/malloc.h>
#include "threads/synch.h"
#include "threads/thread.h"
#include "filesys/file.h"
#include "vm/frame.h"
#include "vm/swap.h"

void lru_init()
{
    lru_clock = NULL;
    lock_init(&lru_lock);
    list_init(&lru_list);
}

struct page *alloc_page(enum palloc_flags flags)
{
    // PAL_USER 플래그가 없는 경우 바로 반환
    if (!(flags & PAL_USER))
        return NULL;

    // 물리 메모리 페이지 할당
    void *kpage = palloc_get_page(flags);
    while (!kpage)
    {
        evict_page();
        kpage = palloc_get_page(flags);
    }

    // page 구조체 메모리 할당
    struct page *page = malloc(sizeof(struct page));
    if (!page)
    {
        // 할당 실패 시 이미 할당 받은 페이지 반환
        palloc_free_page(kpage);
        return NULL;
    }

    // 구조체 초기화
    page->kernel_addr = kpage;
    page->holder = thread_current();

    // LRU 리스트에 삽입
    lock_acquire(&lru_lock);
    list_push_back(&lru_list, &page->lru_elem);
    lock_release(&lru_lock);

    return page;
}

void evict_page()
{
    lock_acquire(&lru_lock);

    // LRU 리스트가 비었으면 바로 반환
    if (list_empty(&lru_list))
    {
        lock_release(&lru_lock);
        return;
    }

    // lru_clock을 다음 페이지로 이동 (원형으로 돌아가며)
    if (!lru_clock)
    {
        lru_clock = list_entry(list_begin(&lru_list), struct page, lru_elem);
    }
    else
    {
        struct list_elem *next_e = list_next(&lru_clock->lru_elem);
        if (next_e == list_end(&lru_list))
            next_e = list_begin(&lru_list);
        lru_clock = list_entry(next_e, struct page, lru_elem);
    }

    // LRU 리스트 전체를 한 바퀴 순회
    int tries = list_size(&lru_list);
    for (int i = 0; i < tries; i++)
    {
        struct page *page = lru_clock;
        struct thread *thread = page->holder;

        bool is_dirty = pagedir_is_dirty(thread->pagedir, page->vm_entry->base_vaddr);
        bool is_accessed = pagedir_is_accessed(thread->pagedir, page->vm_entry->base_vaddr);

        if (is_accessed)
        {
            // 접근된 페이지의 accessed 비트를 지우고 다음 페이지로 이동
            pagedir_set_accessed(thread->pagedir, page->vm_entry->base_vaddr, false);

            // 다음 페이지로 이동 (원형)
            struct list_elem *next_e = list_next(&lru_clock->lru_elem);
            if (next_e == list_end(&lru_list))
                next_e = list_begin(&lru_list);
            lru_clock = list_entry(next_e, struct page, lru_elem);
            continue; // 현재 페이지는 아직 제거할 수 없으므로 다음 루프로 이동
        }

        // 접근되지 않은 페이지를 발견했으므로 제거(evict) 시도
        // 더티 페이지거나 VM_ENTRY가 특정 타입이면 swap에 저장
        if (page->vm_entry->type || is_dirty)
        {
            page->vm_entry->type = VM_PAGE_SWAP;
            page->vm_entry->swap_index = swap_save(page->kernel_addr);
        }

        // 페이지 디스크립터 정리
        page->vm_entry->is_loaded = false;
        pagedir_clear_page(thread->pagedir, page->vm_entry->base_vaddr);
        palloc_free_page(page->kernel_addr);

        // LRU 리스트에서 제거
        struct list_elem *removed_e = list_remove(&page->lru_elem);

        // lru_clock 업데이트
        if (!list_empty(&lru_list))
        {
            // 제거된 페이지가 가리키던 다음 요소로 lru_clock 이동
            if (removed_e == list_end(&lru_list))
                removed_e = list_begin(&lru_list);
            lru_clock = list_entry(removed_e, struct page, lru_elem);
        }
        else
        {
            // 리스트가 비었다면 lru_clock은 NULL
            lru_clock = NULL;
        }

        free(page);
        break; // 한 페이지만 제거하고 종료
    }

    lock_release(&lru_lock);
}

void release_page(void *kaddr)
{
    // NULL 주소는 처리할 필요 없음
    if (kaddr == NULL)
        return;

    lock_acquire(&lru_lock);

    struct list_elem *e;
    for (e = list_begin(&lru_list); e != list_end(&lru_list); e = list_next(e))
    {
        struct page *page = list_entry(e, struct page, lru_elem);

        // 현재 페이지가 해제하려는 주소와 일치하지 않으면 다음 반복으로
        if (page->kernel_addr != kaddr)
            continue;

        // 페이지 실제 메모리 해제
        palloc_free_page(page->kernel_addr);

        // LRU 리스트에서 제거하고, 제거한 후 다음 요소의 위치를 반환
        struct list_elem *next_elem = list_remove(&page->lru_elem);

        // lru_clock이 제거한 페이지를 가리키고 있었다면 업데이트 필요
        if (lru_clock == page)
        {
            if (!list_empty(&lru_list))
            {
                // 만약 next_elem이 리스트의 끝이라면 다시 처음으로
                if (next_elem == list_end(&lru_list))
                    next_elem = list_begin(&lru_list);

                lru_clock = list_entry(next_elem, struct page, lru_elem);
            }
            else
            {
                // 리스트가 비었으면 lru_clock은 NULL
                lru_clock = NULL;
            }
        }

        // page 구조체 메모리 해제
        free(page);

        // 작업 완료
        lock_release(&lru_lock);
        return;
    }

    lock_release(&lru_lock);
    // 여기까지 왔다면 해당 kaddr를 가진 페이지를 찾지 못한 것.
}
