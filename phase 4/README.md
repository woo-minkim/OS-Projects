# Phase 4 상세 README

## 1) 단계 개요
- 이 단계는 가상메모리(VM) 계층을 추가해 lazy loading, page fault 처리, frame eviction, swap in/out 흐름을 연결한 구현이다.
- 핵심 파일은 `vm/*`, `userprog/process.c`, `userprog/exception.c`, `userprog/syscall.c`다.

## 2) 주요 구현 내용 (코드 기준)
- VM 엔트리/페이지 자료구조 (`src/vm/page.h`, `src/threads/thread.h`)
- `virtual_page`(파일 백업 정보, swap index, writable, is_loaded, base_vaddr)와
- `page`(kernel frame 주소, LRU list 노드, 소유 스레드, vm_entry)를 사용한다.
- 스레드마다 `hash vm`을 유지해 가상주소 -> VM 엔트리 매핑을 관리한다.

- Lazy Loading (`src/userprog/process.c`)
- `load_segment()`에서 즉시 물리 페이지를 올리지 않고 VM 엔트리를 해시 테이블에 등록한다.
- 실제 접근 시 page fault가 발생하면 그때 파일/스왑에서 데이터를 적재한다.

- Page Fault 처리 + Stack Growth (`src/userprog/exception.c`, `src/userprog/process.c`)
- `page_fault()`에서 user 주소/`not_present` 조건을 우선 검사한다.
- 해당 주소의 VM 엔트리가 있으면 `memory_fault_handler()`로 로딩/매핑을 수행한다.
- VM 엔트리가 없으면 스택 확장 경로를 검사한다:
  - `fault_addr >= esp - 32`
  - 최대 스택 크기(`MAX_STACK_SIZE`, 8MB) 이내
- 조건 충족 시 새 스택 페이지를 생성하고 VM 해시에 등록한다.

- Frame 관리 + Eviction (`src/vm/frame.c`)
- 전역 `lru_list`와 `lru_clock`(clock 알고리즘)으로 희생 페이지를 찾는다.
- `accessed` 비트가 켜진 페이지는 second chance를 주고,
- dirty 페이지 또는 스왑 대상 페이지는 `swap_save()` 후 매핑을 해제한다.

- Swap 관리 (`src/vm/swap.c`)
- 스왑 블록 + bitmap으로 슬롯 사용 여부를 관리한다.
- `swap_save()`가 페이지를 슬롯에 저장하고 인덱스를 반환,
- `swap_load()`가 인덱스 기반으로 페이지를 메모리로 복원한다.

- 종료 시 VM 정리 (`src/userprog/process.c`)
- `hash_destroy(&cur->vm, destroy_vm)`로 VM 엔트리를 순회 정리한다.
- 로드된 페이지는 `release_page()`와 `pagedir_clear_page()`로 해제한다.

- syscall 보강 (`src/userprog/syscall.c`)
- 사용자 문자열 복사(`copy_string_from_user`)로 `open()` 인자 안정성을 높였다.
- `write()` 경로에서 `buffer_isvalid()`로 사용자 버퍼와 VM 엔트리를 추가 검증한다.
- `create()` 등 파일시스템 접근은 락 보호를 적용했다.

## 3) syscall/기능 상태 메모
- 구현 syscall은 Phase 2 계열(`open/create/read/write/...`) 중심이며,
- `SYS_MMAP`, `SYS_MUNMAP` 번호/유저 헤더 선언은 존재하지만 syscall handler에 해당 case 구현은 없다.

## 4) 폴더 구조
- `src/`
- `document.docx`
- `addition/`
- `README.md`

## 5) addition 보관 항목
- `20212019/`
- `os_prj4_20212019.tar.gz`
- `Proj4.pdf`
