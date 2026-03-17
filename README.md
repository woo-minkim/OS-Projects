# Pintos 프로젝트 정리본 (Phase 1~4)

이 저장소는 Pintos 과제 진행 결과를 `phase 1`부터 `phase 4`까지 단계별로 정리한 버전입니다.  
각 phase는 같은 폴더 규칙을 따르며, 코드(`src`)와 대표 보고서(`document.docx`)를 바로 확인할 수 있도록 구성했습니다.

## 공통 폴더 규칙

`phase 1`~`phase 4`의 최상위에는 아래 4개만 둡니다.

- `src/`: 해당 phase 코드
- `document.docx`: 해당 phase 대표 보고서
- `addition/`: 참고자료, 압축본, 부가 문서, 임시/원본 자료
- `README.md`: 해당 phase 설명

## Phase 요약

- `phase 1`: Threads 기초(스케줄링/동기화 기반) 단계 코드
- `phase 2`: User Program 및 System Call 확장 단계 코드
- `phase 3`: File System 기능 확장 단계 코드
- `phase 4`: Virtual Memory 확장 단계 코드

세부 내용은 각 phase의 `README.md`를 참고하세요.

## 정리 원칙

- 압축 파일(`.tar`, `.tar.gz`)에서 필요한 `src`/문서를 확보한 뒤 루트 기준 구조로 정규화
- 보고서 파일명은 `document.docx`로 통일
- 그 외 자료(`pdf`, 원본 압축, 보조 문서, 중간 결과물)는 `addition/`으로 이동
- 코드 내용은 변경하지 않고 파일 구조와 문서화만 재정리

## 빠른 확인 방법

1. `phase n/src`에서 코드 확인
2. `phase n/document.docx`에서 보고서 확인
3. `phase n/addition`에서 부가 자료 확인

## 빌드/실행 참고

Pintos 일반 흐름 기준 예시:

```bash
cd "phase 1/src/threads"
make
```

phase별 상세 빌드 위치와 구현 포인트는 각 phase README를 참고하면 됩니다.
