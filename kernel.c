#include "kernel.h"
#include "common.h"

/* linker script에 작성한 것과 동일한 변수명이어야 함 */
extern char __kernel_base[];
extern char __bss[], __bss_end[], __stack_top[];
extern char __free_ram[], __free_ram_end[];

/* shell.bin.o 에 포함된 심볼 */
extern char _binary_shell_bin_start[], _binary_shell_bin_size[];

void map_page(uint32_t *table1, vaddr_t vaddr, paddr_t paddr, uint32_t flags);
struct process *create_process(const void *image, size_t image_size);
void switch_context(uint32_t *prev_sp, uint32_t *next_sp);
void yield(void);
paddr_t alloc_pages(uint32_t n);
struct sbiret sbi_call(long arg0, long arg1, long arg2, long arg3, long arg4, long arg5, long fid,  long eid);
void putchar(char ch);
void kernel_entry(void);
void handle_trap(struct trap_frame *f);
void delay(void);
void kernel_main(void);


__attribute__((naked))
void user_entry(void) {
    __asm__ __volatile__(
        "csrw sepc, %[sepc]        \n"
        "csrw sstatus, %[sstatus]  \n"
        "sret                      \n"
        :
        : [sepc] "r" (USER_BASE),
          [sstatus] "r" (SSTATUS_SPIE)
    );
}


//--------------------------------------------------------------
// [page table]

void map_page(uint32_t *table1, vaddr_t vaddr, paddr_t paddr, uint32_t flags)
{
    if (!is_aligned(vaddr, PAGE_SIZE))
        PANIC("unaligned vaddr %x", vaddr);

    if (!is_aligned(paddr, PAGE_SIZE))
        PANIC("unaligned paddr %x", paddr);

    uint32_t vpn1 = (vaddr >> 22) & 0x3ff;
    if ((table1[vpn1] & PAGE_V) == 0)
    {
        uint32_t pt_paddr = alloc_pages(1);
        table1[vpn1] = ((pt_paddr / PAGE_SIZE) << 10) | PAGE_V;
    }

    uint32_t vpn0 = (vaddr >> 12) & 0x3ff;
    uint32_t *table0 = (uint32_t *)((table1[vpn1] >> 10) * PAGE_SIZE);
    table0[vpn0] = ((paddr / PAGE_SIZE) << 10) | flags | PAGE_V;
}




//--------------------------------------------------------------
// [process]

struct process procs[PROCS_MAX];        // 모든 프로세스를 모아 두는 배열  

struct process *current_proc;           // 현재 실행 중인 process 
struct process *idle_proc;              // idle process


struct process *create_process(const void *image, size_t image_size)
{
    struct process *proc = NULL;
    int i;
    for(i = 0; i < PROCS_MAX; i++)
    {
        if (procs[i].state == PROC_UNUSED)
        {
            proc = &procs[i];
            break;
        }
    }

    if (!proc)
        PANIC("no free process slots");

    // 커널 스택에 callee-saved 레지스터 공간을 미리 준비
    // 첫 컨텍스트 스위치 시, switch_context에서 이 값들을 복원함
    uint32_t *sp = (uint32_t *) &proc->stack[sizeof(proc->stack)];
    *--sp = 0;                      // s11
    *--sp = 0;                      // s10
    *--sp = 0;                      // s9
    *--sp = 0;                      // s8
    *--sp = 0;                      // s7
    *--sp = 0;                      // s6
    *--sp = 0;                      // s5
    *--sp = 0;                      // s4
    *--sp = 0;                      // s3
    *--sp = 0;                      // s2
    *--sp = 0;                      // s1
    *--sp = 0;                      // s0
    *--sp = (uint32_t) user_entry;  // ra (처음 실행 시 점프할 주소) user application 실행

    // map kernel page
    uint32_t *page_table = (uint32_t *) alloc_pages(1);
    for (paddr_t paddr = (paddr_t) __kernel_base;
    paddr < (paddr_t) __free_ram_end; paddr += PAGE_SIZE)
    map_page(page_table, paddr, paddr, PAGE_R | PAGE_W | PAGE_X);

    // map user page
    for (uint32_t off = 0; off < image_size; off += PAGE_SIZE)
    {
        paddr_t page = alloc_pages(1);

        size_t remaining = image_size - off;
        size_t copy_size = PAGE_SIZE <= remaining ? PAGE_SIZE : remaining;

        memcpy((void *) page, image + off, copy_size);
        map_page(page_table, USER_BASE + off, page, PAGE_U | PAGE_R | PAGE_W | PAGE_X);
    }

    // 구조체 필드 초기화
    proc->pid = i + 1;
    proc->state = PROC_RUNNABLE;
    proc->sp = (uint32_t) sp;
    proc->page_table = page_table;
    return proc;
}




/*
 * callee-saved reg : 함수를 호출된 쪽에서 보존 책임을 지는 레지스터 (s0-11)
 * caller-saved reg : 함수를 호출하는 곳에서 보존 책임을 가지는 레지스터 (a0-7, t0-6, ra)
 * 자세한 건 calling convention 참고
 *
 * 결국 context switching이란 현재 프로세스의 실행 상태(레지스터 값들)를 저장하고, 
 * 다른 프로세스로 제어를 넘긴 후, 
 * 나중에 다시 이 프로세스로 돌아왔을 때 저장해둔 상태를 복원하는 과정이다.
 */
__attribute__((naked))
void switch_context(uint32_t *prev_sp, uint32_t *next_sp)
{
    __asm__ __volatile__(
            "addi sp, sp, -13 * 4\n"
            "sw ra,  0  * 4(sp)\n" 
            "sw s0,  1  * 4(sp)\n"
            "sw s1,  2  * 4(sp)\n"
            "sw s2,  3  * 4(sp)\n"
            "sw s3,  4  * 4(sp)\n"
            "sw s4,  5  * 4(sp)\n"
            "sw s5,  6  * 4(sp)\n"
            "sw s6,  7  * 4(sp)\n"
            "sw s7,  8  * 4(sp)\n"
            "sw s8,  9  * 4(sp)\n"
            "sw s9,  10 * 4(sp)\n"
            "sw s10, 11 * 4(sp)\n"
            "sw s11, 12 * 4(sp)\n"

            // 스택 포인터 교체
            "sw sp, (a0)\n"         // *prev_sp = sp
            "lw sp, (a1)\n"         // sp = *next_sp

            // 다음 프로세스 스택에서 callee-saved 레지스터 복원
            "lw ra,  0  * 4(sp)\n"  
            "lw s0,  1  * 4(sp)\n"
            "lw s1,  2  * 4(sp)\n"
            "lw s2,  3  * 4(sp)\n"
            "lw s3,  4  * 4(sp)\n"
            "lw s4,  5  * 4(sp)\n"
            "lw s5,  6  * 4(sp)\n"
            "lw s6,  7  * 4(sp)\n"
            "lw s7,  8  * 4(sp)\n"
            "lw s8,  9  * 4(sp)\n"
            "lw s9,  10 * 4(sp)\n"
            "lw s10, 11 * 4(sp)\n"
            "lw s11, 12 * 4(sp)\n"
            "addi sp, sp, 13 * 4\n" 
            "ret\n"
            );
}

// yield 함수는 "이 프로세스에서 다른 프로세스로 context switching하겠다. 어떤 프로세스가 실행될지는 스케쥴러 소관이고" 의 의미
// 현재는 yield를 호출할 때만 context switching이 일어나므로 co-operational multitasking임  
void yield(void)
{
    // 간단한 프로세스 스케쥴링.
    // 현재 PID에서 시작해 원형으로 돌아가며 검색하는 방식
    struct process *next = idle_proc;
    for(int i = 0; i < PROCS_MAX; i++)
    {
        struct process *proc = &procs[(current_proc->pid + i) % PROCS_MAX];
        if (proc->state == PROC_RUNNABLE && proc->pid > 0)
        {
            next = proc;
            break;
        }
    }

    if (next == current_proc)
        return;

    __asm__ __volatile__(
        "sfence.vma\n"
        "csrw satp, %[satp]\n"
        "sfence.vma\n"
        "csrw sscratch, %[sscratch]\n"
        :
        :[satp] "r" (SATP_SV32 | ((uint32_t) next->page_table / PAGE_SIZE)),
         [sscratch] "r" ((uint32_t)&next->stack[sizeof(next->stack)])
    );

    struct process *prev = current_proc;
    current_proc = next;

    // 실질적인 context switching  
    switch_context(&prev->sp, &next->sp);
}

//--------------------------------------------------------------
// [memory alloc]
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

//--------------------------------------------------------------
// [function call]
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

//--------------------------------------------------------------
// [exception handler]
// 예외 발생한 sp를 임시 저장 (sscratch = s-mode scratch)  
// 범용 register 를 모두 저장  
// exception handler 트리거  
// 이후 복원 절차 수행  
    __attribute__((naked))
    __attribute__((aligned(4)))
void kernel_entry(void)
{
    __asm__ __volatile__(
            "csrrw sp, sscratch, sp\n"

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

            "addi a0, sp, 4 * 31\n"
            "csrw sscratch, a0\n"

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


void delay(void)
{
    for(int i = 0; i < 30000000; i++)
    {
        __asm__ __volatile__("nop"); // do nothing
    }
}

struct process *proc_a;
struct process *proc_b;

void proc_a_entry(void)
{
    printf("starting process A\n");
    while (1)
    {
        putchar('A');
        yield();
    }
}

void proc_b_entry(void)
{
    printf("starting process B\n");
    while (1)
    {
        putchar('B');
        yield();
    }
}

void kernel_main(void)
{
    memset(__bss, 0, (size_t) __bss_end - (size_t) __bss);

    printf("\n\n");

    WRITE_CSR(stvec, (uint32_t) kernel_entry);

    idle_proc = create_process(NULL, 0);
    idle_proc->pid = 0; // idle
    current_proc = idle_proc;

    create_process(_binary_shell_bin_start, (size_t) _binary_shell_bin_size);

    yield();
    PANIC("switched to idle process");

    //WRITE_CSR(stvec, (uint32_t)kernel_entry);
    //__asm__ __volatile__("unimp");

    //paddr_t paddr0 = alloc_pages(2);
    //paddr_t paddr1 = alloc_pages(1);
    //printf("alloc_pages test: paddr0=%x\n", paddr0);
    //printf("alloc_pages test: paddr1=%x\n", paddr1);

    //PANIC("booted!");
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
