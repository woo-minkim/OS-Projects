#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "userprog/pagedir.h"
#include "userprog/process.h"
#include "userprog/syscall.h"
#include <threads/malloc.h>
#include <threads/palloc.h>
#include "threads/vaddr.h"
#include "threads/thread.h"
#include "threads/synch.h"
#include "lib/kernel/list.h"
#include "filesys/file.h"
#include "vm/page.h"
#include "vm/frame.h"

/*
 * 주어진 가상 주소(virtual_address)에 해당하는 VM 엔트리를 해시 테이블에서 검색해서 반환한다.
 * 해시 검색 시 페이지 단위 정렬(pg_round_down)로 페이지 시작 주소를 맞춘다.
 */
struct virtual_page *find_virtual_page(void *virtual_address)
{
    // 페이지 단위로 주소를 내림 정렬하여 해시 키로 사용할 엔트리 준비
    struct virtual_page temp_entry;
    temp_entry.base_vaddr = pg_round_down(virtual_address);

    // 현재 스레드의 VM 해시 테이블에서 해당 엔트리 검색
    struct hash_elem *found_elem = hash_find(&thread_current()->vm, &temp_entry.elem);

    // 검색 실패 시
    if (found_elem == NULL)
        return NULL;

    // 검색 성공 시 해시 요소를 실제 virtual_page 구조체로 변환하여 반환
    return hash_entry(found_elem, struct virtual_page, elem);
}

/*
 * 파일로부터 페이지에 필요한 데이터를 읽어 메모리에 로드한다.
 * read_bytes 만큼 파일에서 읽고, 남은 zero_bytes만큼을 0으로 채운다.
 */
bool load_page_data(void *kernel_addr, struct virtual_page *vm_entry)
{
    size_t bytes_read = file_read_at(vm_entry->backing_file, kernel_addr, vm_entry->read_bytes, vm_entry->file_offset);
    if (bytes_read != vm_entry->read_bytes)
        return false;

    /* zero_bytes 만큼 메모리를 0으로 채운다. */
    uint8_t *dst = (uint8_t *)kernel_addr + vm_entry->read_bytes;
    memset(dst, 0, vm_entry->zero_bytes);

    return true;
}

/*
 * 새로운 VM 엔트리(vme)를 해시 테이블(vm_table)에 삽입한다.
 * hash_insert가 NULL을 반환하면 삽입 성공을 의미한다.
 */
bool insert_virtual_page(struct hash *vm_table, struct virtual_page *vm_entry)
{
    if (vm_table == NULL || vm_entry == NULL)
        return false;

    // 해시 테이블에 엔트리 삽입
    struct hash_elem *existing_elem = hash_insert(vm_table, &vm_entry->elem);

    if (existing_elem == NULL)
        return true;
    else
        return false;
}
