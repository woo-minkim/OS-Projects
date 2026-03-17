# Phase 3 상세 README

## 1) 단계 개요
- 이 단계는 Threads 스케줄링 고도화(Alarm Clock, Priority Scheduling, MLFQS/aging 보강)가 핵심이다.
- 코드 기준으로 `threads/*`, `devices/timer.c`의 변화가 크고, `userprog/syscall.c` 계층은 Phase 2 구조를 대부분 유지한다.

## 2) 주요 구현 내용 (코드 기준)
- Alarm Clock (`src/devices/timer.c`, `src/threads/thread.h`)
- `sleep_list` + 스레드별 `wake_ticks`를 사용한다.
- `timer_sleep()`는 현재 스레드를 block 상태로 넣고,
- `timer_interrupt()`에서 깨울 시점이 된 스레드를 `thread_unblock()`으로 복귀시킨다.

- Priority Scheduling (`src/threads/thread.c`, `src/threads/synch.c`)
- `ready_list` 삽입 시 우선순위 순서대로 정렬 삽입한다.
- `thread_create()`, `thread_yield()`, `thread_set_priority()`에서 선점 조건을 반영한다.
- `sema_up()`은 대기열에서 최대 우선순위 스레드를 깨우는 방식으로 동작한다.

- Priority Aging + MLFQS (`src/threads/thread.c`)
- `prior_aging` 플래그 기반 aging 함수(`priority_aging`)가 추가돼 대기 스레드 우선순위를 주기적으로 증가시킨다.
- `load_average`, `recent_cpu`, `niceness`를 사용해 MLFQS 계산 경로를 구현한다.
- `thread_set_nice()`, `thread_get_nice()`, `thread_get_load_avg()`, `thread_get_recent_cpu()`가 동작하도록 연결되어 있다.

- 구조 확장 (`src/threads/thread.h`)
- 스레드 구조체에 `wake_ticks`, `niceness`, `recent_cpu` 필드가 추가됐다.

## 3) 단계 범위 관련 메모
- `userprog`/`filesys` 계층은 Phase 2 대비 큰 구조 변화가 없다.
- 특히 `filesys/inode.c` 기준으로 파일 길이 자동 확장(path growth)은 여전히 제한적이다.

## 4) 폴더 구조
- `src/`
- `document.docx`
- `addition/`
- `README.md`

## 5) addition 보관 항목
- `20212019/`
- `threads_tests_extracted/`
- `os_prj3_20212019.tar.gz`
- `threads_tests.tar`
- `proj3.pdf`
- `pintos안되던거.docx`
