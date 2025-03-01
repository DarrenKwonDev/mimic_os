# mimic os

mimicking some OS features using qemu-system-riscv32  

```text
kernel.c (소스 코드) → 컴파일/링크 → kernel.elf (실행 파일) → QEMU 에뮬레이션 → 실행 및 디버깅 
    ↑                                  ↑  
    |                                  |  
 kernel.ld ----------------------------|  
(링커 스크립트)  
```

```text
응용 프로그램 (User mode, U-mode)  
          ↑↓
운영체제 커널 (Supervisor mode, S-mode) - os code. in kernel code  
          ↑↓ [ECALL을 통한 SBI 호출]  
SBI/SEE (Machine mode, M-mode 또는 Hypervisor mode, H-mode) - OpenSBI같은 펌웨어  
          ↑↓
하드웨어 (실제 RISC-V 프로세서)  
```

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


ra (x1): Return Address - 함수 호출 후 돌아갈 주소를 저장  
sp (x2): Stack Pointer - 현재 스택의 최상단을 가리킴  
gp (x3): Global Pointer - 전역 변수에 쉽게 접근하기 위한 포인터  
tp (x4): Thread Pointer - 스레드 로컬 저장소를 가리킴  
t0-t6 (x5-x7, x28-x31): Temporary registers - 임시 값 저장용, 함수 호출 시 보존되지 않음  
s0-s11 (x8-x9, x18-x27): Saved registers - 함수 호출 간에 보존되어야 하는 값 저장  
a0-a7 (x10-x17): Argument/Return registers - 함수 인자 및 반환값 전달에 사용  

## exception

```text
CPU가 medeleg 레지스터 확인 (예외 처리 모드 결정)
        ↓
CSR에 상태 저장
    - scause: 예외 유형
    - stval: 예외 부가 정보
    - sepc: 예외 발생 PC 값
    - sstatus: 예외 발생 시 운영 모드
        ↓
stvec 레지스터 값으로 PC 변경 (예외 핸들러 주소로 점프)
        ↓
커널 예외 핸들러 실행
        ↓
일반 레지스터 상태 저장
        ↓
예외 처리 로직 수행 (scause로 예외 유형 확인)
        ↓
저장했던 상태 복원
        ↓
sret 명령어로 예외 발생 지점으로 복귀
        ↓
프로그램 실행 재개
```

## callee, caller saved reg

| Register  | ABI Name | Description                     | Saver  |   
|-----------|----------|---------------------------------|--------|  
| x0        | zero     | Hard-wired zero                 | —     |  
| x1        | ra       | Return address                  | Caller |   
| x2        | sp       | Stack pointer                   | Callee |   
| x3        | gp       | Global pointer                  | —     |   
| x4        | tp       | Thread pointer                  | —     |   
| x5-7      | t0-2     | Temporaries                     | Caller |   
| x8        | s0/fp    | Saved register/frame pointer    | Callee |   
| x9        | s1       | Saved register                  | Callee |   
| x10-11    | a0-1     | Function arguments/return values| Caller |   
| x12-17    | a2-7     | Function arguments              | Caller |   
| x18-27    | s2-11    | Saved registers                 | Callee |   
| x28-31    | t3-6     | Temporaries                     | Caller |   
| f0-7      | ft0-7    | FP temporaries                  | Caller |   
| f8-9      | fs0-1    | FP saved registers              | Callee |  
| f10-11    | fa0-1    | FP arguments/return values      | Caller |   
| f12-17    | fa2-7    | FP arguments                    | Caller |   
| f18-27    | fs2-11   | FP saved registers              | Callee |   
| f28-31    | ft8-11   | FP temporaries                  | Caller |    
