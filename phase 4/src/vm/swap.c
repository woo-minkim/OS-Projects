#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "bitmap.h"
#include "threads/synch.h"
#include "devices/block.h"
#include "vm/frame.h"
#include "vm/page.h"
#include "vm/swap.h"

/* 스왑 페이지를 구성하는 섹터 개수:
   Pintos 설정 상 한 페이지 = 4KB, 한 섹터 = 512바이트이므로
   4KB / 512B = 8섹터 */
#define SECTORS_PER_PAGE 8
/* 스왑 디스크를 가리키는 block 포인터 */
static struct block *swap_block;
/* 스왑 영역 사용 상태를 추적하는 비트맵 */
static struct bitmap *swap_bitmap;
/* 스왑 영역 접근 보호를 위한 락 */
static struct lock swap_lock;

/*
 * swap_load:
 * swap_index에 해당하는 스왑 슬롯로부터 물리 메모리 영역 physical_addr로 페이지를 읽어온다.
 * 읽은 뒤 해당 스왑 인덱스를 비워준다(bitmap_flip).
 */
void swap_load(size_t swap_index, void *physical_addr)
{
    if (physical_addr == NULL)
        return; // physical_addr가 NULL이면 아무 작업 없음.

    lock_acquire(&swap_lock);

    size_t total_slots = bitmap_size(swap_bitmap);
    if (swap_index >= total_slots)
    {
        // swap_index가 스왑 범위를 벗어나면 반환
        lock_release(&swap_lock);
        return;
    }

    // 스왑 슬롯이 실제로 사용 중인지 확인
    bool in_use = bitmap_test(swap_bitmap, swap_index);
    if (!in_use)
    {
        // 해당 슬롯이 사용 중이지 않다면 로드할 필요 없음.
        lock_release(&swap_lock);
        return;
    }

    // 스왑 슬롯의 시작 섹터 인덱스 계산 (한 페이지당 SECTORS_PER_PAGE 섹터)
    size_t base_sector = swap_index * SECTORS_PER_PAGE;
    uint8_t *dest = (uint8_t *)physical_addr;

    // 스왑 디스크 -> 물리 주소로 페이지 데이터 복사
    for (size_t i = 0; i < SECTORS_PER_PAGE; i++)
    {
        size_t sector_idx = base_sector + i;
        void *sector_addr = dest + (i * BLOCK_SECTOR_SIZE);
        block_read(swap_block, sector_idx, sector_addr);
    }

    // 해당 스왑 슬롯을 빈 상태로 만든다.
    bitmap_set(swap_bitmap, swap_index, false);
    lock_release(&swap_lock);
}

/*
 * swap_save:
 * 물리 메모리에 있는 페이지를 스왑 영역에 기록한다.
 * 사용할 수 있는 스왑 슬롯을 찾아 페이지를 스왑 디스크에 기록하고,
 * 해당 스왑 인덱스를 반환한다.
 */
size_t swap_save(void *physical_addr)
{
    // physical_addr가 NULL인 경우 스왑 불가
    if (physical_addr == NULL)
        return BITMAP_ERROR;

    lock_acquire(&swap_lock);

    // 스왑 비트맵에서 비어있는 슬롯을 하나 찾고, 해당 슬롯을 사용중으로 flip
    size_t swap_index = bitmap_scan_and_flip(swap_bitmap, 0, 1, false);

    // 사용 가능한 스왑 슬롯을 찾지 못한 경우 즉시 반환
    if (swap_index == BITMAP_ERROR)
    {
        // printf("No free swap slot available.\n");
        lock_release(&swap_lock);
        return BITMAP_ERROR;
    }

    // 찾은 스왑 슬롯의 시작 섹터 인덱스(한 페이지 = SECTORS_PER_PAGE 섹터)
    size_t base_sector = swap_index * SECTORS_PER_PAGE;
    // 쓰기 원본(물리 주소) 시작점
    uint8_t *src = (uint8_t *)physical_addr;

    // 물리 메모리 -> 스왑 디스크로 페이지 데이터 기록
    // 한 페이지는 SECTORS_PER_PAGE 개의 섹터로 나뉨
    for (size_t i = 0; i < SECTORS_PER_PAGE; i++)
    {
        size_t sector_idx = base_sector + i;
        void *sector_addr = src + (i * BLOCK_SECTOR_SIZE);
        // 스왑 디스크에 페이지 데이터 기록
        block_write(swap_block, sector_idx, sector_addr);
    }

    lock_release(&swap_lock);
    return swap_index; // 성공 시 스왑 인덱스, 실패 시 BITMAP_ERROR
}

/*
 * swap_init:
 * 스왑 디스크를 얻고, 해당 스왑 디스크에 맞는 비트맵을 생성한 뒤
 * 전부 빈 상태로 초기화한다. 또한 스왑 락을 초기화한다.
 */
void swap_init()
{
    // 스왑 락 초기화
    lock_init(&swap_lock);

    // 스왑 블록 가져오기
    swap_block = block_get_role(BLOCK_SWAP);
    if (!swap_block)
        return;

    // 스왑 크기 계산 및 유효성 검사
    size_t block_size_bytes = block_size(swap_block);
    if (block_size_bytes < SECTORS_PER_PAGE)
        return;

    size_t swap_size = block_size_bytes / SECTORS_PER_PAGE;
    if (swap_size == 0)
        return;

    // 비트맵 생성
    swap_bitmap = bitmap_create(swap_size);
    if (!swap_bitmap)
        return;

    // 비트맵 초기화
    bitmap_set_all(swap_bitmap, false);
}
