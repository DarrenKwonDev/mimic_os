#!/bin/bash

#x: 디버깅 모드 활성화 (eXecution trace). 실행되는 모든 명령어를 실행 전에 화면에 출력합니다. 명령어 앞에는 + 기호가 붙습니다. 스크립트가 어떻게 실행되는지 추적하는 데 유용합니다.
#u: 정의되지 않은 변수 사용 시 오류 발생 (Undefined variable). 값이 설정되지 않은 변수를 사용하려고 하면 스크립트가 즉시 종료됩니다. 이는 방금 발견한 QUMU/QEMU 변수 불일치 같은 오류를 잡는 데 도움이 됩니다.
#e: 명령어 실패 시 즉시 종료 (Error). 명령어가 0이 아닌 종료 코드를 반환하면(실패를 의미) 스크립트 실행이 즉시 중단됩니다.
set -xue

QEMU=qemu-system-riscv32

OBJCOPY=/usr/bin/llvm-objcopy

# clang 경로와 컴파일 옵션
CC=/usr/bin/clang
CFLAGS="-std=c11 -O2 -g3 -Wall -Wextra --target=riscv32-unknown-elf -fno-stack-protector -ffreestanding -nostdlib"

# 사용자 applicaiton을 ELF 형식으로 컴파일  
$CC $CFLAGS -Wl,-Tuser.ld -Wl,-Map=shell.map -o shell.elf shell.c user.c common.c

# ELF 파일을 순수 바이너리(.bin)로 변환 - 메타데이터 제거하고 실행 코드와 데이터만 추출
# .bss 섹션(초기화되지 않은 데이터)도 출력에 포함시킴
$OBJCOPY --set-section-flags .bss=alloc,contents -O binary shell.elf shell.bin

# 순수 바이너리(.bin)를 오브젝트 파일(.o)로 변환 - 커널에서 참조할 수 있는 심볼 생성
# _binary_shell_bin_start, _binary_shell_bin_end, _binary_shell_bin_size 심볼이 자동 생성됨
$OBJCOPY -Ibinary -Oelf32-littleriscv shell.bin shell.bin.o


# 커널 빌드
$CC $CFLAGS -Wl,-Tkernel.ld -Wl,-Map=kernel.map -o kernel.elf \
    kernel.c \
    common.c \
    shell.bin.o

#-machine virt: virt 머신을 시작합니다. -machine '?' 명령어로 다른 머신 종류를 확인할 수 있습니다.
#-bios default: QEMU가 제공하는 기본 펌웨어(OpenSBI)를 사용합니다.
#-nographic: GUI 없이 QEMU를 실행합니다.
#-serial mon:stdio: QEMU의 표준 입출력을 가상 머신의 시리얼 포트에 연결합니다. 
#                   이를 통해 가상 머신에서 출력하는 텍스트가 호스트의 터미널에 표시되고,
#                   호스트의 키보드 입력이 가상 머신으로 전달됩니다.
#                   mon: 접두사를 붙여 Ctrl+A 이후 C를 눌러 QEMU 모니터로 전환할 수 있습니다.
#--no-reboot: 가상 머신이 크래시되면 재부팅하지 않고 종료합니다(디버깅 시에 편리합니다).
$QEMU -machine virt \
      -bios default \
      -nographic \
      -serial mon:stdio \
      --no-reboot \
      -kernel kernel.elf
