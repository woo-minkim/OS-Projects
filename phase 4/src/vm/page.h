#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <hash.h>
#include <stdbool.h>
#include "filesys/file.h"

/* 페이지 타입 정의: 파일로부터 로드되는 페이지인지, 스왑 영역에서 로드되는 페이지인지 나타낸다. */
enum vm_page_type
{
    VM_PAGE_FILE,
    VM_PAGE_SWAP
};
/* 물리 프레임과 연동되는 페이지 구조체: LRU 리스트 관리 시 사용 */
struct page
{
    void *kernel_addr;             // 커널 가상 주소
    struct list_elem lru_elem;     // LRU 리스트 엘리먼트
    struct frame *physical_frame;  // 페이지가 할당된 물리 프레임
    struct thread *holder;         // 해당 페이지를 소유하는 스레드
    struct virtual_page *vm_entry; // 해당 페이지에 대응되는 VM 엔트리
};

/* 가상 메모리 엔트리 구조체: 프로세스의 VM 해시 테이블에 저장되는 엔트리.
   하나의 유저 가상 페이지(upage)에 해당하며, 그 페이지가 어디서 어떻게 로드되는지 정보를 담는다. */
struct virtual_page
{
    /* 백업 파일 관련 필드 */
    struct file *backing_file; // 해당 페이지를 백업하는 파일.
    unsigned long file_offset; // 백업 파일 내 오프셋.
    unsigned long read_bytes;  // 파일로부터 읽어올 바이트 수.
    unsigned long zero_bytes;  // 메모리에 0으로 채울 바이트 수.

    /* 스왑 영역 관련 필드 */
    unsigned long swap_index; // 스왑 슬롯 인덱스 (스왑 영역에 있을 때 유효).

    /* 페이지 상태 및 속성 */
    enum vm_page_type type; // 페이지 타입 (파일/스왑).
    bool dirty;             // 페이지가 메모리에 로드된 후 수정되었는지 여부.
    bool writable;          // 해당 메모리 영역이 쓰기 가능한지 여부.
    bool is_loaded;         // 페이지가 물리 메모리에 로드되어 있는지 여부.

    void *base_vaddr; // 가상 메모리 엔트리가 담당하는 페이지의 기준 주소 (upage와 동일한 용도로 사용 가능).

    /* 해시 테이블 요소 */
    struct hash_elem elem;
};

struct virtual_page *find_virtual_page(void *virtual_address);
bool load_page_data(void *kernel_addr, struct virtual_page *vm_entry);
bool insert_virtual_page(struct hash *vm_table, struct virtual_page *vm_entry);

#endif /* VM_PAGE_H */
