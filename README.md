# mimic os

mimicking some OS features using qemu-system-riscv32  

## features  

NOT YET   

## tree  

├── disk/ - 파일 시스템용 디렉터리   
├── common.c - 커널/유저 공용 라이브러리(printf, memset 등)  
├── common.h - 커널/유저 공용 라이브러리용 구조체 및 상수 정의  
├── kernel.c - 커널(프로세스 관리, 시스템 콜, 디바이스 드라이버, 파일 시스템 등)  
├── kernel.h - 커널용 구조체 및 상수 정의  
├── kernel.ld - 커널용 링커 스크립트(메모리 레이아웃 정의)  
├── shell.c - 명령줄 셸  
├── user.c - 유저 라이브러리(시스템 콜 관련 함수)  
├── user.h - 유저 라이브러리용 구조체 및 상수 정의  
├── user.ld - 유저용 링커 스크립트(메모리 레이아웃 정의)  
└── run.sh - 빌드 스크립트  


## QEMU shortcut  

C-a h    도움말 표시  
C-a x    에뮬레이터 종료  
C-a s    디스크 데이터를 파일에 저장(-snapshot 사용 시)  
C-a t    콘솔 타임스탬프 토글  
C-a b    break(매직 sysrq)   
C-a c    콘솔과 모니터 간 전환   
C-a C-a  C-a를 전송  

## RISC-V asm  

| Register    | ABI Name (alias) | Description
|-------------|------------------|----------------------------------------------|
| pc          | pc               | 프로그램 카운터 (다음 실행할 명령어 주소)
| x0          | zero             | 항상 0인 레지스터
| x1          | ra               | 함수 호출에서 복귀 주소 저장
| x2          | sp               | 스택 포인터
| x5 - x7     | t0 - t2          | 임시용 레지스터 (임의 용도로 사용 가능)
| x8          | fp               | 스택 프레임 포인터
| x10 - x11   | a0 - a1          | 함수 인자 및 반환값
| x12 - x17   | a2 - a7          | 함수 인자
| x18 - x27   | s0 - s11         | 함수 호출 사이에도 값이 보존되는 레지스터
| x28 - x31   | t3 - t6          | 임시용 레지스터



