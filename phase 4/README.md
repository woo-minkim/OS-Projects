# Phase 4. Virtual Memory and Demand Paging

Phase 4에서는 Pintos에 **가상 메모리(Virtual Memory) 계층을 도입해 demand paging 흐름을 구현**했습니다.  
`vm/*` 모듈과 `userprog`의 예외 처리 및 로딩 경로를 연결해 lazy loading, page fault handling, stack growth, frame eviction, swap in/out까지 하나의 메모리 관리 흐름으로 확장했습니다.

## Overview

이 단계의 핵심 목표는 프로그램 로딩 시 모든 페이지를 즉시 올리지 않고  
**실제 접근 시점에 페이지를 적재하는 demand paging 기반 실행 환경을 만드는 것**이었습니다.

- VM entry 관리
- lazy loading
- page fault handling
- stack growth
- frame eviction
- swap in / swap out
- syscall 버퍼 / 문자열 검증 보강

## Technical Focus

Phase 4에서 중점적으로 다룬 부분은 다음과 같습니다.

- **Demand Paging**: 필요한 시점에만 페이지를 적재하는 lazy loading 구조
- **Page Fault Handling**: fault 원인에 따라 기존 VM entry 로딩 또는 stack growth 수행
- **Frame Management**: 물리 프레임 부족 시 victim 선정 및 eviction
- **Swap System**: evicted page를 디스크에 저장하고 재접근 시 복구
- **User Memory Validation**: syscall 인자와 버퍼 접근의 안전성 보강

## Subsystems

### 1. VM Entry and Hash-based Lookup
**Files**
- `src/vm/page.h`
- `src/vm/page.c`
- `src/threads/thread.h`

**Key Points**
- `struct virtual_page`는 backing file 정보, `swap_index`, `writable`, `is_loaded`, `base_vaddr`를 관리합니다.
- `struct page`는 커널 프레임 주소(`kernel_addr`)와 LRU 연결 정보(`lru_elem`)를 관리합니다.
- 각 스레드 구조체에 `struct hash vm`을 두고, `find_virtual_page()`와 `insert_virtual_page()`를 통해 VM entry를 조회 / 등록합니다.

이 구조를 통해 프로세스별로 **가상 주소를 기준으로 페이지 상태를 추적하는 인덱스 계층**을 구성했습니다.

### 2. Lazy Loading
**File**
- `src/userprog/process.c`

**Key Points**
- `load_segment()`는 실행 파일 로딩 시 즉시 물리 페이지를 할당하지 않고 `virtual_page`만 VM hash에 등록합니다.
- 실제 접근이 발생하면 `page_fault()` → `memory_fault_handler()` 경로에서 물리 프레임을 할당하고 데이터를 적재합니다.

이 단계에서는 프로그램 시작 시 전체 메모리를 채우는 방식 대신 = 
**필요한 페이지를 필요한 시점에만 메모리에 올리는 구조**를 구현했습니다.

### 3. Page Fault Handling and Stack Growth
**Files**
- `src/userprog/exception.c`
- `src/userprog/process.h`
- `src/userprog/process.c`

**Key Points**
- `page_fault()`는 먼저 `not_present` 여부와 user address 조건을 확인합니다.
- 기존 VM entry가 존재하면 `memory_fault_handler()`를 통해 페이지를 적재합니다.
- VM entry가 없는 경우에는 stack growth 조건(`fault_addr >= esp - 32`, `MAX_STACK_SIZE` 이내)을 검사해 새로운 페이지를 생성합니다.

이를 통해 fault 원인에 따라 **기존 파일 기반 페이지 로딩과 스택 확장을 분기 처리**하도록 구현했습니다.

### 4. Frame Eviction with Clock Policy
**File**
- `src/vm/frame.c`

**Key Points**
- `alloc_page()`는 사용 가능한 프레임이 없을 경우 `evict_page()`를 호출합니다.
- `evict_page()`는 `lru_list`와 `lru_clock`을 기반으로 clock replacement policy를 적용합니다.
- `pagedir_is_accessed()` 비트가 설정된 페이지는 second chance를 주고, dirty 또는 swap 대상 페이지는 `swap_save()`를 통해 디스크로 내립니다.

이 단계에서는 물리 메모리 부족 시 **LRU 근사 기반의 frame replacement 흐름**을 구성했습니다.

### 5. Swap In / Swap Out
**File**
- `src/vm/swap.c`

**Key Points**
- `swap_init()`은 swap block, bitmap, lock을 초기화합니다.
- `swap_save()`는 비어 있는 swap 슬롯을 찾아 페이지 내용을 디스크 sector에 기록합니다.
- `swap_load()`는 swap 슬롯의 내용을 다시 프레임으로 읽어오고, 해당 bitmap을 해제합니다.

이를 통해 물리 메모리를 초과하는 페이지를 **보조 저장소로 밀어내고 다시 복구하는 swap 계층**을 구현했습니다.

### 6. Syscall-side Memory Validation
**File**
- `src/userprog/syscall.c`

**Key Points**
- `copy_string_from_user()`는 `open()` 인자 문자열을 커널 버퍼로 안전하게 복사합니다.
- `buffer_isvalid()`는 `write()` 버퍼의 사용자 주소 및 VM entry 유효성을 검사합니다.
- 파일 관련 syscall은 `filesys_lock`으로 보호됩니다.

이 단계에서는 VM 도입 이후 syscall 경로에서도  
**유저 메모리 접근이 안전하게 이뤄지도록 검증 로직을 보강**했습니다.

## Core Data Structures

- `struct hash vm`  
  프로세스별 VM entry를 관리하는 해시 기반 인덱스입니다.

- `lru_list`, `lru_clock`, `lru_lock`  
  frame replacement 대상과 clock 포인터 상태를 관리하는 구조입니다.

- `swap_bitmap`, `swap_lock`  
  swap 슬롯 사용 여부와 동기화를 관리하는 구조입니다.

## Execution Flow

1. `load_segment()`가 실행 파일의 각 페이지를 즉시 적재하지 않고 `virtual_page`로만 등록합니다.
2. 프로그램 실행 중 페이지 접근 시 `page_fault()`가 발생하면 VM entry를 확인합니다.
3. 기존 entry가 있으면 `memory_fault_handler()`가 프레임 할당 및 매핑을 수행합니다.
4. 메모리가 부족하면 `evict_page()`가 victim을 선정하고, 필요 시 `swap_save()`를 호출합니다.
5. swap된 페이지에 다시 접근하면 `swap_load()`를 통해 내용을 복구합니다.
6. 프로세스 종료 시 `hash_destroy(&cur->vm, destroy_vm)`를 호출해 VM entry를 정리합니다.

## Implemented Features

| Category | Item | Status |
| --- | --- | --- |
| Basic Control | `halt`, `exit`, `exec`, `wait` | Implemented |
| I/O | `read`, `write` | Implemented (with stronger buffer validation) |
| File Syscalls | `create`, `remove`, `open`, `filesize`, `seek`, `tell`, `close` | Implemented |
| Project Extensions | `fibonacci`, `max_of_four_int` | Implemented |
| VM Core | lazy loading, page fault handling, stack growth, frame eviction, swap | Implemented |
| Memory Mapping API | `SYS_MMAP`, `SYS_MUNMAP` | syscall number / user wrapper only, handler case not implemented |

## What I Built

- 프로세스별 VM hash를 구성해 가상 주소 단위로 페이지 상태를 추적하도록 구현했습니다.
- 실행 파일 로딩 시 lazy loading 방식을 적용해 즉시 물리 페이지를 할당하지 않도록 변경했습니다.
- `page_fault()` 경로에서 VM entry 로딩과 stack growth를 분기 처리했습니다.
- frame 부족 시 clock 기반 eviction 정책을 적용하고, swap 계층과 연결해 페이지를 저장 / 복구하도록 구현했습니다.
- syscall 인자 문자열 및 버퍼 검증을 보강해 VM 도입 이후에도 사용자 메모리 접근이 안전하게 이뤄지도록 했습니다.
