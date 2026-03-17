# Pintos Projects

Pintos 기반 운영체제 프로젝트를 단계별로 정리한 저장소입니다.  
기본 스레드 실행 구조부터 사용자 프로그램, 시스템콜, 스케줄링, 가상 메모리까지 **운영체제의 핵심 기능을 Phase 1~4에 걸쳐 확장 구현한 결과**를 담고 있습니다.

## Overview

이 저장소는 Pintos 과제를 4개 Phase로 나누어 정리한 프로젝트 모음입니다.  
각 Phase는 이전 단계의 기능을 바탕으로 운영체제의 주요 기능을 점진적으로 확장하는 방식으로 구성되어 있습니다.

- **Phase 1**: 기본 스레드 실행 구조와 사용자 프로그램 실행 기반
- **Phase 2**: 시스템콜 확장과 파일 디스크립터 관리
- **Phase 3**: Alarm Clock, Priority Scheduling, MLFQS 등 스케줄러 구현
- **Phase 4**: Lazy Loading, Page Fault Handling, Stack Growth, Swap 기반 가상 메모리 구현

## Phase Navigation

- [Phase 1 README](phase%201/README.md)
- [Phase 2 README](phase%202/README.md)
- [Phase 3 README](phase%203/README.md)
- [Phase 4 README](phase%204/README.md)

## Project Structure

```text
Pintos Projects/
├─ phase 1/
│  ├─ src/
│  ├─ document.docx
│  ├─ addition/
│  └─ README.md
├─ phase 2/
│  ├─ src/
│  ├─ document.docx
│  ├─ addition/
│  └─ README.md
├─ phase 3/
│  ├─ src/
│  ├─ document.docx
│  ├─ addition/
│  └─ README.md
└─ phase 4/
   ├─ src/
   ├─ document.docx
   ├─ addition/
   └─ README.md