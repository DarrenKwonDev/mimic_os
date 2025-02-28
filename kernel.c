#include "kernel.h"
#include "common.h"

typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef uint32_t size_t;

/* linker script에 작성한 것과 동일한 변수명이어야 함 */
extern char __bss[], __bss_end[], __stack_top[];

struct sbiret sbi_call(long arg0, long arg1, long arg2, long arg3, long arg4,
                        long arg5, long fid,  long eid) 
{
    register long a0 __asm__("a0") = arg0;
    register long a1 __asm__("a1") = arg1;
    register long a2 __asm__("a2") = arg2;
    register long a3 __asm__("a3") = arg3;
    register long a4 __asm__("a4") = arg4;
    register long a5 __asm__("a5") = arg5;
    register long a6 __asm__("a6") = fid;
    register long a7 __asm__("a7") = eid;

    // ECALL is used as the control transfer instruction between the supervisor and the SEE.
    __asm__ __volatile__("ecall"
                        : "=r"(a0), "=r"(a1)
                        : "r"(a0), "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5), "r"(a6), "r"(a7)
                        : "memory");

    return (struct sbiret){.error = a0, .value = a1};
}

void putchar(char ch)
{
    sbi_call(ch, 0, 0, 0, 0, 0, 0, 1);
}

void *memset(void *buf, char c, size_t n)
{
    uint8_t *p = (uint8_t *)buf;
    while(n--)
        *p++ = c;
    return buf;
}

void kernel_main(void)
{
    printf("\n\nHello %s\n", "World!");
    printf("1 + 2 = %d, %x \n", 1 + 2, 0x1234abcd);

    for (;;) {
        // Wait For Interrupt
        __asm__ __volatile__("wfi");
    }
}

// 컴파일러에게 __attribute__를 통해 명령.
// section은 linker script에 등록한 섹션에 해당 함수를 배치하라.
// naked는 컴파일러가 함수에 대한 프롤로그(함수 시작 시 스택 프레임 설정 등)와
//                                에필로그(함수 종료 시 정리 작업) 코드를 생성하지 않음
__attribute__((section(".text.boot"))) 
__attribute__((naked))
void boot(void) // linker script로 ENTRY에 등록함 
{
    // RISC-V ISA
    /*
        __asm__ __volatile__("assembly code" 
                            : output operands 
                            : input operands 
                            : clobbered registers);
    */
    __asm__ __volatile__(
        "mv sp, %[stack_top]\n"         // sp = stack_pop   
        "j kernel_main\n"               // jump to kernel_main func
        :
        : [stack_top] "r" (__stack_top) 
    );  // clobbered registers 부분이 생략됨.
}
