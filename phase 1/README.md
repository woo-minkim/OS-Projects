# Phase 1. User Program Execution Foundation

Phase 1에서는 Pintos에서 **사용자 프로그램을 실행할 수 있는 최소 기반**을 구현했습니다.  
스레드 실행의 기본 흐름 위에 사용자 프로세스 로딩, 인자 스택 구성, 부모-자식 동기화, 기본 syscall 처리 경로를 연결해 이후 phase 확장의 출발점을 마련했습니다.

## Overview

이 단계의 핵심 목표는 고급 스케줄링이나 파일 시스템 확장보다  
**유저 프로그램이 정상적으로 로드되고 실행되며, 커널과 기본 상호작용을 수행할 수 있는 구조를 만드는 것**이었습니다.

- 사용자 프로그램 실행 및 ELF 로딩
- 초기 사용자 스택 구성
- 부모-자식 프로세스 동기화
- 기본 syscall 처리
- 스레드 실행 및 ready queue 기반 스케줄링 흐름 유지

## Technical Focus

Phase 1에서 중점적으로 다룬 부분은 다음과 같습니다.

- **Thread Execution Flow**: runnable 상태 전환과 CPU 양보 흐름
- **User Program Loading**: ELF 실행 파일 로딩과 사용자 스택 초기화
- **Parent-Child Synchronization**: 로딩 완료와 종료 상태 회수를 위한 동기화
- **Basic System Call Handling**: 사용자 프로그램과 커널 간 최소 syscall 인터페이스 연결

## Subsystems

### 1. Thread Scheduling Basics
**File**
- `src/threads/thread.c`

**Key Points**
- `thread_unblock()`은 `ready_list`에 스레드를 넣어 runnable 상태를 복구합니다.
- `thread_yield()`는 현재 스레드를 다시 `ready_list`에 넣고 CPU를 양보합니다.
- `thread_set_priority()`는 우선순위를 단순 갱신하는 형태로 동작합니다.
- `thread_get_nice()`, `thread_get_load_avg()`, `thread_get_recent_cpu()`는 인터페이스만 유지되며, 고급 스케줄링 로직은 아직 포함하지 않았습니다.

이 단계에서는 고급 정책보다 **스레드가 실행 가능 상태로 전환되고 스케줄러에 다시 진입하는 기본 흐름**을 유지하는 데 초점을 두었습니다.

### 2. Timer Sleep Behavior
**File**
- `src/devices/timer.c`

**Key Points**
- `timer_sleep()`은 별도의 sleep queue 없이 `thread_yield()`를 반복하는 방식으로 대기합니다.
- 이후 Phase 3에서 Alarm Clock 방식으로 보완되기 전의 기본 형태입니다.

이 구현은 효율적인 sleep 관리보다는 **최소 동작 보장**에 초점을 둔 초기 단계입니다.

### 3. Process Execution, Loading, and Waiting
**File**
- `src/userprog/process.c`

**Key Points**
- `process_execute()`는 커맨드라인에서 실행 파일명을 분리하고 `thread_create()`로 새 실행 흐름을 시작합니다.
- `load()`는 ELF를 로드한 뒤 사용자 스택에 `argc`, `argv`, null sentinel, return address를 직접 구성합니다.
- `start_process()`는 `load_sema`를 사용해 부모와 자식 사이의 로딩 완료를 동기화합니다.
- `process_wait()`는 부모의 `child_List`를 탐색해 자식 종료 상태를 회수합니다.

이 단계에서 핵심은 **프로세스 생성부터 유저 코드 진입까지의 경로를 실제로 연결하는 것**이었습니다.

### 4. Basic System Call Handling
**File**
- `src/userprog/syscall.c`

**Key Points**
- `syscall_handler()`가 syscall 번호에 따라 각 기능으로 분기합니다.
- `validate_user_addr()`를 통해 syscall 인자 주소를 검증합니다.
- `read()`는 `fd == 0`(stdin), `write()`는 `fd == 1`(stdout)을 중심으로 처리합니다.
- 과제 확장 항목인 `fibonacci()`, `max_of_four_int()`를 함께 구현했습니다.

이 단계에서는 파일 관련 syscall 전체보다는 **기본 제어 및 입출력 syscall 경로를 우선 연결**했습니다.

## Core Data Structures

- `ready_list`  
  실행 대기 상태의 스레드를 관리하는 큐입니다.

- `child_List`  
  부모 스레드가 자식 프로세스 정보를 추적하기 위한 리스트입니다.

- `exit_sema`, `load_sema`, `remove_sema`  
  부모-자식 간 로딩 완료, 종료 상태 전달, 정리 시점을 동기화하기 위한 세마포어입니다.

## Execution Flow

1. `process_execute()`가 실행 파일명을 파싱하고 새 스레드를 생성합니다.
2. `start_process()`가 `load()`를 호출해 ELF와 사용자 스택을 준비합니다.
3. 사용자 프로그램 실행 중 syscall trap이 발생하면 `syscall_handler()`가 이를 처리합니다.
4. 종료 시 `exit()` / `process_exit()`가 상태를 기록하고 부모의 `process_wait()`가 종료 상태를 회수합니다.

## Implemented Features

| Category | Item | Status |
| --- | --- | --- |
| Basic Control | `halt`, `exit`, `exec`, `wait` | Implemented |
| I/O | `read`, `write` | Implemented (`stdin` / `stdout` 중심) |
| File Syscalls | `create`, `remove`, `open`, `filesize`, `seek`, `tell`, `close` | Not yet implemented |
| Project Extensions | `fibonacci`, `max_of_four_int` | Implemented |

## What I Built

- 사용자 프로그램 실행을 위한 프로세스 생성, ELF 로딩, 초기 사용자 스택 구성을 구현했습니다.
- 부모와 자식 프로세스 간 로딩 완료 및 종료 상태 회수를 위한 동기화 구조를 만들었습니다.
- syscall 번호 분기와 사용자 주소 검증을 포함한 기본 syscall 처리 경로를 연결했습니다.
- `stdin` / `stdout` 기반 입출력과 필수 제어 syscall을 우선 구현해 최소 실행 환경을 완성했습니다.

## Why This Phase Matters

Phase 1은 고급 스케줄링이나 가상 메모리 구현 이전에,  
**유저 프로그램이 커널 위에서 실제로 실행될 수 있는 최소 운영체제 기반을 만드는 단계**입니다.

이 단계에서 구축한 실행, 로딩, 동기화, syscall 흐름은 이후 Phase 2~4에서  
파일 시스템, 스케줄링, 가상 메모리 기능으로 확장되는 기반이 됩니다.