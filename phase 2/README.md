# Phase 2. File System Syscalls and Per-Thread FD Management

Phase 2에서는 Phase 1에서 만든 사용자 프로그램 실행 기반 위에  
**파일 시스템 syscall 계층과 스레드별 파일 디스크립터(FD) 관리 구조를 확장 구현**했습니다.  
`open`, `create`, `read`, `write`, `close` 등 파일 관련 syscall을 연결하고 파일 접근 동기화와 프로세스 종료 시 자원 정리까지 포함해 사용자 프로그램 지원 범위를 넓혔습니다.

## Overview

이 단계의 핵심 목표는 사용자 프로그램이 파일 시스템과 상호작용할 수 있도록 syscall 계층을 확장하고    
**각 스레드가 독립적으로 파일 디스크립터를 관리할 수 있는 구조를 만드는 것**이었습니다.

- 파일 관련 syscall 구현
- 스레드별 FD 테이블 관리
- 파일 시스템 접근 임계구역 보호
- 부모-자식 간 로딩 / 종료 동기화 보강

## Technical Focus

Phase 2에서 중점적으로 다룬 부분은 다음과 같습니다.

- **Per-thread File Descriptor Table**: 스레드 단위 FD 관리
- **File System Syscalls**: 파일 생성, 열기, 읽기, 쓰기, 닫기 지원
- **Kernel Synchronization**: 전역 파일 시스템 lock을 통한 임계구역 보호
- **Process Lifecycle Management**: 로딩 성공 여부 및 종료 상태 동기화

## Subsystems

### 1. Per-thread FD Table Management
**Files**
- `src/threads/thread.h`
- `src/userprog/syscall.c`

**Key Points**
- 각 스레드 구조체에 `fd[128]`, `fd_size`, `fd_nxt`를 두어 파일 디스크립터를 관리했습니다.
- `open()`은 사용 가능한 빈 슬롯을 찾아 새 FD를 할당합니다.
- `close()`는 FD 슬롯을 회수하고 `fd_nxt`를 갱신합니다.
- `exit()`에서는 열린 FD를 순회하며 정리합니다.

이 구조를 통해 각 스레드가 **독립적인 파일 디스크립터 집합을 갖고 파일 객체를 참조**할 수 있도록 만들었습니다.

### 2. File-related Syscall Implementation
**File**
- `src/userprog/syscall.c`

**Key Points**
- `create()`, `remove()`, `open()`, `filesize()`, `seek()`, `tell()`, `close()`를 구현했습니다.
- `read()`와 `write()`는 `stdin` / `stdout`뿐 아니라 일반 파일 FD 경로까지 처리하도록 확장했습니다.
- 현재 실행 중인 자기 바이너리에 대한 쓰기를 방지하기 위해 `open()`에서 `file_deny_write()`를 적용했습니다.

이 단계에서 syscall 계층은 단순 입출력을 넘어 **실제 파일 시스템 객체를 다루는 인터페이스**로 확장되었습니다.

### 3. File System Critical Section Protection
**File**
- `src/userprog/syscall.c`

**Key Points**
- 전역 `filesys_lock`을 `syscall_init()`에서 초기화했습니다.
- `read()`, `write()`, `open()` 등 파일 시스템 접근이 발생하는 경로에서 lock acquire / release를 수행했습니다.

이를 통해 파일 시스템 계층에 대한 동시 접근 시 **커널 내부 자료구조가 일관되게 유지되도록 보호**했습니다.

### 4. Parent-Child Synchronization Reinforcement
**File**
- `src/userprog/process.c`

**Key Points**
- `process_execute()`는 `load_sema`를 사용해 로드 성공 / 실패 여부를 부모에게 동기화합니다.
- `process_wait()`는 `exit_sema`, `remove_sema`를 활용해 자식 종료 상태를 회수합니다.

이 단계에서는 단순 실행을 넘어서 **프로세스 생명주기와 종료 상태 전달이 더 명확히 정리된 구조**를 갖추도록 보강했습니다.

## Core Data Structures

- `fd[128]`, `fd_size`, `fd_nxt`  
  스레드별 파일 디스크립터 테이블과 관리 정보입니다.

- `filesys_lock`  
  파일 시스템 syscall 경로를 직렬화하기 위한 전역 lock입니다.

- `child_List`, `load_sema`, `exit_sema`, `remove_sema`  
  부모-자식 프로세스 간 로딩과 종료 상태를 동기화하기 위한 구조입니다.

## Execution Flow

1. `syscall_handler()`가 syscall 번호를 해석하고 분기합니다.
2. 파일 관련 syscall은 `filesys_lock` 보호 구간 안에서 `filesys_*`, `file_*` 계층을 호출합니다.
3. `open()`은 FD 테이블에 새 슬롯을 배정하고 `read()`, `write()`, `seek()`, `tell()`, `close()`는 이를 통해 파일 객체에 접근합니다.
4. 프로세스 종료 시 `exit()`가 열린 FD를 정리하고 부모는 `process_wait()`에서 종료 상태를 회수합니다.

## Implemented Features

| Category | Item | Status |
| --- | --- | --- |
| Basic Control | `halt`, `exit`, `exec`, `wait` | Implemented |
| I/O | `read`, `write` | Implemented |
| File Syscalls | `create`, `remove`, `open`, `filesize`, `seek`, `tell`, `close` | Implemented |
| Project Extensions | `fibonacci`, `max_of_four_int` | Implemented |
| VM Mapping | `mmap`, `munmap` | Not yet implemented |

## What I Built

- 사용자 프로그램이 파일 시스템과 상호작용할 수 있도록 파일 관련 syscall 경로를 구현했습니다.
- 스레드별 FD 테이블을 구성해 파일 객체를 프로세스 단위가 아닌 스레드 단위에서 추적하고 관리했습니다.
- 전역 `filesys_lock`을 적용해 파일 시스템 접근 구간을 보호했습니다.
- 부모-자식 간 로드 성공 여부와 종료 상태 회수 흐름을 보강해 프로세스 생명주기 관리 구조를 정리했습니다.
- 실행 중인 자기 바이너리에 대한 쓰기를 제한해 기본적인 실행 파일 보호 정책도 반영했습니다.

## Why This Phase Matters

Phase 2는 사용자 프로그램이 단순히 실행되는 수준을 넘어  
**파일 시스템을 실제로 사용하고 커널 자원을 정리하며 종료할 수 있는 운영체제 환경을 갖추는 단계**입니다.

이 단계에서 구현한 FD 관리, 파일 syscall, 동기화 구조는 이후 Phase 4의  
가상 메모리 및 메모리 매핑 기능으로 확장되는 중요한 기반이 됩니다.