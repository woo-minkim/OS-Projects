# Phase 2 상세 README

## 1) 단계 개요
- Phase 1의 사용자 프로그램 실행 기반 위에 파일시스템 syscall과 파일 디스크립터 테이블을 확장한 단계다.
- 핵심 변경 축은 `userprog/syscall.c`, `userprog/process.c`, `threads/thread.h`다.

## 2) 주요 구현 내용 (코드 기준)
- 파일 디스크립터 관리 (`src/threads/thread.h`, `src/userprog/syscall.c`)
- 스레드별 `fd[128]`, `fd_size`, `fd_nxt`를 사용해 FD를 관리한다.
- FD 정책: `0` stdin, `1` stdout, 일반 파일은 `3~127` 범위로 할당.
- `open()`은 첫 빈 슬롯을 찾아 할당하고, 실행 중인 자기 바이너리 파일은 `file_deny_write()` 처리한다.

- 시스템콜 확장 (`src/userprog/syscall.c`)
- 기본 syscall 외 파일시스템 관련 syscall 구현:
  - `create`, `remove`, `open`, `filesize`, `seek`, `tell`, `close`
- `filesys_lock` 전역 락으로 파일시스템 접근 임계구역을 보호한다.
- `exit()`에서 열려 있는 파일 디스크립터를 순회 종료한다.

- 프로세스 동기화/부모-자식 관계 (`src/userprog/process.c`)
- `process_execute()`에서 로딩 완료를 `load_sema`로 동기화한다.
- 로딩 실패 시 부모가 감지해 실패 경로로 처리한다.
- `process_wait()`는 자식 리스트 탐색 후 `exit_sema`/`remove_sema` 기반으로 종료 상태를 수거한다.

- 사용자 포인터 검증 (`src/userprog/syscall.c`)
- `validate_user_addr()`로 syscall 스택 인자 주소와 사용자 포인터를 검증한다.

## 3) syscall 구현 목록
- 프로세스/기본: `halt`, `exit`, `exec`, `wait`
- I/O: `read`, `write`
- 파일시스템: `create`, `remove`, `open`, `filesize`, `seek`, `tell`, `close`
- 과제 확장: `fibonacci`, `max_of_four_int`

## 4) 현재 코드의 한계/주의
- 메모리 매핑(`mmap/munmap`)은 구현 대상이 아니다.
- 디렉터리 확장/VM 기반 fault handling은 다음 단계에서 확장된다.

## 5) 폴더 구조
- `src/`
- `document.docx`
- `addition/`
- `README.md`

## 6) addition 보관 항목
- `os_prj2_20212019/`
- `os_prj2_20212019.tar.gz`
- `proj2.pdf`
