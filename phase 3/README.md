# Phase 3. Advanced Thread Scheduling

Phase 3에서는 Pintos의 `threads` 서브시스템을 확장해  
**Alarm Clock, priority scheduling, priority-aware synchronization, MLFQS 기반 스케줄링**을 구현했습니다.  
기존 Phase 2의 사용자 프로그램 및 파일 시스템 기능은 유지한 채 커널 스케줄러의 정책과 실행 흐름을 고도화하는 데 집중했습니다.

## Overview

이 단계의 핵심 목표는 단순 round-robin 수준의 스레드 실행을 넘어서 
**우선순위와 tick 기반 상태 갱신을 반영하는 스케줄러를 구현하는 것**이었습니다.

- busy waiting 없는 sleep / wakeup 구현
- priority 기반 ready queue 관리
- synchronization primitive에서 우선순위 반영
- MLFQS 및 priority aging 경로 구현

## Technical Focus

Phase 3에서 중점적으로 다룬 부분은 다음과 같습니다.

- **Alarm Clock**: sleep queue 기반 block / wakeup 처리
- **Priority Scheduling**: 우선순위 기반 runnable thread 관리
- **Priority-aware Synchronization**: semaphore 대기열에서 높은 priority 스레드 우선 해제
- **MLFQS**: `nice`, `recent_cpu`, `load_avg`를 반영한 동적 priority 계산
- **Preemption Control**: 우선순위 변화나 tick 갱신에 따른 선점 유도

## Subsystems

### 1. Alarm Clock
**Files**
- `src/devices/timer.c`
- `src/threads/thread.h`

**Key Points**
- `timer_sleep()`은 현재 스레드의 `wake_ticks`를 설정하고 `sleep_list`에 넣은 뒤 block합니다.
- `timer_interrupt()`는 매 tick마다 `sleep_list`를 확인하고, 기상 시점에 도달한 스레드를 `thread_unblock()`합니다.

이 구현을 통해 Phase 1의 반복 `yield` 기반 대기 대신 
**busy waiting 없이 sleep queue를 이용한 효율적인 대기 방식**을 구성했습니다.

### 2. Priority Scheduling
**Files**
- `src/threads/thread.c`
- `src/threads/synch.c`

**Key Points**
- `thread_unblock()`과 `thread_yield()`는 스레드를 `ready_list`에 priority 순으로 삽입합니다.
- `thread_set_priority()`는 우선순위 변경 시 현재 실행 중인 스레드가 CPU를 양보해야 하는지 판단합니다.
- `sema_up()`은 대기자 중 가장 높은 priority를 가진 스레드를 깨우도록 동작합니다.

이 단계에서는 runnable queue뿐 아니라 synchronization 경로에도  
**우선순위 기반 정책이 일관되게 반영되도록 구현**했습니다.

### 3. MLFQS and Priority Aging
**File**
- `src/threads/thread.c`

**Key Points**
- `priority_aging()`은 `all_list`를 순회하며 장시간 대기 스레드의 priority를 조정합니다.
- `mlfqs_update_load_avg()`, `mlfqs_recalc_recent_cpu()`, `mlfqs_recalc_priority()`는 MLFQS 계산 경로를 담당합니다.
- `thread_mlfqs_tick()`은 tick 주기에 따라 `recent_cpu`, `load_avg`, priority를 갱신합니다.
- `thread_set_nice()`, `thread_get_nice()`, `thread_get_load_avg()`, `thread_get_recent_cpu()` 인터페이스가 실제 계산 로직과 연결됩니다.

이를 통해 정적 priority만 사용하는 방식이 아니라,  
**시스템 부하와 CPU 사용 이력을 반영해 priority가 동적으로 갱신되는 스케줄링 구조**를 구현했습니다.

## Core Data Structures

- `sleep_list`, `wake_ticks`  
  sleep 상태인 스레드와 각 기상 시점을 관리하는 구조입니다.

- `ready_list`  
  runnable 상태의 스레드를 priority 순으로 유지하는 큐입니다.

- `niceness`, `recent_cpu`, `load_average`  
  MLFQS 계산에 사용되는 스케줄링 상태 값입니다.

## Execution Flow

1. `timer_sleep()`을 호출한 스레드는 `sleep_list`에 들어가고 block됩니다.
2. `timer_interrupt()`가 tick마다 `sleep_list`를 확인해 기상 시점이 된 스레드를 `thread_unblock()`합니다.
3. runnable 스레드는 `ready_list`에 priority 순으로 유지됩니다.
4. `thread_mlfqs_tick()`이 주기적으로 `recent_cpu`, `load_avg`, priority를 갱신하고 필요 시 선점을 유도합니다.

## Implemented Features

| Category | Item | Status |
| --- | --- | --- |
| Basic Control | `halt`, `exit`, `exec`, `wait` | Implemented |
| I/O | `read`, `write` | Implemented |
| File Syscalls | `create`, `remove`, `open`, `filesize`, `seek`, `tell`, `close` | Implemented |
| Project Extensions | `fibonacci`, `max_of_four_int` | Implemented |
| Advanced Thread Scheduling | Alarm Clock, Priority Scheduling, MLFQS, Aging | Implemented |

## What I Built

- `timer_sleep()` / `timer_interrupt()` 경로를 수정해 sleep queue 기반 Alarm Clock을 구현했습니다.
- `ready_list`를 priority 순으로 유지하도록 스케줄러 흐름을 조정했습니다.
- `thread_set_priority()`와 `sema_up()`에 우선순위 정책을 반영해 선점 및 synchronization 동작이 일관되게 이어지도록 구현했습니다.
- `load_avg`, `recent_cpu`, `nice`를 기반으로 priority를 동적으로 재계산하는 MLFQS 경로를 연결했습니다.
- aging 로직을 추가해 장시간 대기 스레드가 계속 밀리지 않도록 보완했습니다.

## Why This Phase Matters

Phase 3는  
**커널이 어떤 스레드를 언제 실행할지 결정하는 스케줄링 정책 자체를 구현하는 단계**입니다.

이 단계에서 다룬 sleep / wakeup, priority queue, synchronization, MLFQS는  
운영체제의 반응성과 공정성을 좌우하는 핵심 요소이며 이후 VM이나 더 복잡한 커널 기능에서도 중요한 기반이 됩니다.