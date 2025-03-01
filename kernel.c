#include "kernel.h"
#include "common.h"

typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef uint32_t size_t;

/* linker script에 작성한 것과 동일한 변수명이어야 함 */
extern char __bss[], __bss_end[], __stack_top[];
extern char __free_ram[], __free_ram_end[];

paddr_t alloc_pages(uint32_t n)
{
    static paddr_t next_paddr = (paddr_t)__free_ram;
    paddr_t paddr = next_paddr;
    next_paddr += n * PAGE_SIZE;            // 할당합니다.

    if (next_paddr > (paddr_t)__free_ram_end)
        PANIC("out of memory");

    memset((void *)paddr, 0, n * PAGE_SIZE); // 할당한 공간을 zero fill
    return paddr;
}

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


// 예외 발생한 sp를 임시 저장 (sscratch = s-mode scratch)  
// 범용 register 를 모두 저장  
// exception handler 트리거  
// 이후 복원 절차 수행  
__attribute__((naked))
__attribute__((aligned(4)))
void kernel_entry(void)
{
    __asm__ __volatile__(
            "csrw sscratch, sp\n"           // 현재 stack pointer를 sscratch CSR에 write(csrw)  
            "addi sp, sp, -4 * 31\n"        // sp를 현재 위치에서 124바이트(4 * 31)바이트 감소, stack에 31개의 register를 저장할 공간 확보  
            "sw ra,  4 * 0(sp)\n"           // store word(sw) return할 주소를 sp + 0 위치에 저장  
            "sw gp,  4 * 1(sp)\n"           // handler 처리 후 복구할 register를 저장
            "sw tp,  4 * 2(sp)\n"
            "sw t0,  4 * 3(sp)\n"
            "sw t1,  4 * 4(sp)\n"
            "sw t2,  4 * 5(sp)\n"
            "sw t3,  4 * 6(sp)\n"
            "sw t4,  4 * 7(sp)\n"
            "sw t5,  4 * 8(sp)\n"
            "sw t6,  4 * 9(sp)\n"
            "sw a0,  4 * 10(sp)\n"
            "sw a1,  4 * 11(sp)\n"
            "sw a2,  4 * 12(sp)\n"
            "sw a3,  4 * 13(sp)\n"
            "sw a4,  4 * 14(sp)\n"
            "sw a5,  4 * 15(sp)\n"
            "sw a6,  4 * 16(sp)\n"
            "sw a7,  4 * 17(sp)\n"
            "sw s0,  4 * 18(sp)\n"
            "sw s1,  4 * 19(sp)\n"
            "sw s2,  4 * 20(sp)\n"
            "sw s3,  4 * 21(sp)\n"
            "sw s4,  4 * 22(sp)\n"
            "sw s5,  4 * 23(sp)\n"
            "sw s6,  4 * 24(sp)\n"
            "sw s7,  4 * 25(sp)\n"
            "sw s8,  4 * 26(sp)\n"
            "sw s9,  4 * 27(sp)\n"
            "sw s10, 4 * 28(sp)\n"
            "sw s11, 4 * 29(sp)\n"

            "csrr a0, sscratch\n"           // 원래 sp 값을 a0로 read
            "sw a0, 4 * 30(sp)\n"           // 원래 sp값을 stack에 저

            "mv a0, sp\n"                   // 현재 sp 값을 a0에 저장 (handle_trap의 매개 변수로 전달)
            "call handle_trap\n"            // exception handler 호출  

            "lw ra,  4 * 0(sp)\n"           // (load word) 저장된 레지스터들을 복원
            "lw gp,  4 * 1(sp)\n"
            "lw tp,  4 * 2(sp)\n"
            "lw t0,  4 * 3(sp)\n"
            "lw t1,  4 * 4(sp)\n"
            "lw t2,  4 * 5(sp)\n"
            "lw t3,  4 * 6(sp)\n"
            "lw t4,  4 * 7(sp)\n"
            "lw t5,  4 * 8(sp)\n"
            "lw t6,  4 * 9(sp)\n"

            "lw a0,  4 * 10(sp)\n"
            "lw a1,  4 * 11(sp)\n"
            "lw a2,  4 * 12(sp)\n"
            "lw a3,  4 * 13(sp)\n"
            "lw a4,  4 * 14(sp)\n"
            "lw a5,  4 * 15(sp)\n"
            "lw a6,  4 * 16(sp)\n"
            "lw a7,  4 * 17(sp)\n"
            "lw s0,  4 * 18(sp)\n"
            "lw s1,  4 * 19(sp)\n"
            "lw s2,  4 * 20(sp)\n"
            "lw s3,  4 * 21(sp)\n"
            "lw s4,  4 * 22(sp)\n"
            "lw s5,  4 * 23(sp)\n"
            "lw s6,  4 * 24(sp)\n"
            "lw s7,  4 * 25(sp)\n"
            "lw s8,  4 * 26(sp)\n"
            "lw s9,  4 * 27(sp)\n"
            "lw s10, 4 * 28(sp)\n"
            "lw s11, 4 * 29(sp)\n"
            "lw sp,  4 * 30(sp)\n"          // 원래 스택 포인터 복원
            "sret\n"                        // Supervisor Return - 예외 발생 전 위치로 복귀
            );

}

void handle_trap(struct trap_frame *f)
{
    uint32_t scause = READ_CSR(scause);
    uint32_t stval = READ_CSR(stval);
    uint32_t user_pc = READ_CSR(sepc);

    PANIC("unexpected trap scause=%x, stval=%x, sepc=%x\n", scause, stval, user_pc);
}




//--------------------------------------------------------------------------------
// boot and main
void kernel_main(void)
{
    memset(__bss, 0, (size_t) __bss_end - (size_t) __bss);
    
    //WRITE_CSR(stvec, (uint32_t)kernel_entry);
    //__asm__ __volatile__("unimp");
    
    paddr_t paddr0 = alloc_pages(2);
    paddr_t paddr1 = alloc_pages(1);
    printf("alloc_pages test: paddr0=%x\n", paddr0);
    printf("alloc_pages test: paddr1=%x\n", paddr1);

    PANIC("booted!");
    //printf("unreachable here!\n");

    // printf("\n\nHello %s\n", "World!");
    // printf("1 + 2 = %d, %x \n", 1 + 2, 0x1234abcd);

    // for (;;) {
    //     // Wait For Interrupt
    //     __asm__ __volatile__("wfi");
    // }
}

// 컴파일러에게 __attribute__를 통해 명령.
// section은 linker script에 등록한 섹션에 해당 함수를 배치하라.
// naked는 컴파일러가 함수에 대한 프롤로그(함수 시작 시 스택 프레임 설정 등)와
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
