# Phase 1 상세 README

## 1) 단계 개요
- 이 단계의 코드는 `threads` + `userprog` 기초 구현이 함께 포함된 상태다.
- 실제 코드 기준으로 보면 Threads 고급 스케줄링(우선순위 스케줄링/MLFQS)은 대부분 스텁 상태이고, User Program 기본 실행/시스템콜 골격이 먼저 구현되어 있다.

## 2) 주요 구현 내용 (코드 기준)
- 스레드/스케줄러 (`src/threads/thread.c`, `src/threads/synch.c`, `src/devices/timer.c`)
- `ready_list`는 기본 FIFO 기반이고 `thread_set_priority()`는 단순 대입 형태다.
- `thread_get_nice()`, `thread_get_load_avg()`, `thread_get_recent_cpu()`는 미구현(0 반환) 상태다.
- `timer_sleep()`는 sleep queue 없이 `thread_yield()` 반복 방식(바쁜 대기형 양보)이다.

- 프로세스 생성/대기 (`src/userprog/process.c`)
- `process_execute()`에서 커맨드라인 첫 토큰(실행 파일명)을 파싱하고, 파일 존재 확인 후 스레드를 생성한다.
- ELF 로딩 후 사용자 스택 인자 전달(argc/argv/null sentinel/return address) 로직이 들어가 있다.
- `process_wait()`는 부모-자식 리스트(`child_List`)와 세마포어(`exit_sema`, `load_sema`) 기반으로 자식 종료를 수거한다.

- 시스템콜 (`src/userprog/syscall.c`)
- 구현 syscall: `halt`, `exit`, `exec`, `wait`, `read`, `write`, `fibonacci`, `max_of_four_int`.
- 주소 검증은 `validate_user_addr()`에서 `is_user_vaddr` + `pagedir_get_page`로 수행한다.
- `read`는 `fd==0`(stdin), `write`는 `fd==1`(stdout)만 처리한다.

## 3) 현재 코드의 한계/주의
- 파일시스템 syscall(`open/create/remove/...`)은 아직 단계적으로 확장 전 상태다.
- 우선순위 스케줄링/도네이션/MLFQS는 실질 구현 전 단계다.

## 4) 폴더 구조
- `src/`
- `document.docx`
- `addition/`
- `README.md`

## 5) addition 보관 항목
- `os_prj1_20212019/`
- `os_prj1_20212019.tar.gz`
- `2024_os_prj1.pdf`
- `ctags usage guide.pdf`
- `ctags usage guide.pptx`
