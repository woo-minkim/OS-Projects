#ifndef VM_SWAP_H
#define VM_SWAP_H

#include <stddef.h>
#include <stdbool.h>

/* 스왑 테이블 초기화 함수. 스왑 디스크의 크기만큼 비트맵을 생성하고, 모두 빈 상태로 초기화한다. */
void swap_init(void);
/* 스왑 영역에서 swap_index에 해당하는 페이지를 읽어 물리 메모리에 로드한다. */
void swap_load(size_t swap_index, void *physical_addr);
/* 물리 메모리에 있는 페이지를 스왑 영역에 기록하고, 해당 스왑 인덱스를 반환한다. */
size_t swap_save(void *physical_addr);

#endif /* VM_SWAP_H */
