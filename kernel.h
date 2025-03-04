#pragma once

#include "common.h"

// 애플리케이션 이미지의 기본 가상 주소입니다. 이는 `user.ld`에 정의된 시작 주소와 일치해야 합니다.
#define USER_BASE 0x1000000

// do-while(0) 패턴. 
// 세미콜론 문제를 해결하고 매크로 내 break, continue가 외부 루프에 주는 영향을 방지합니다
// 여러 줄로 된 macro를 작성할 때는 관습적으로 작성함  
#define PANIC(fmt, ...)                                                             \
    do {                                                                            \
        printf("PANIC: %s:%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__);       \
        while (1) {}                                                                \
    } while(0)


#define READ_CSR(reg)                                                               \
    ({                                                                              \
        unsigned long __tmp;                                                        \
        __asm__ __volatile__("csrr %0, " #reg : "=r"(__tmp));                       \
        __tmp;                                                                      \
    })

#define WRITE_CSR(reg, value)                                                       \
    do {                                                                            \
        uint32_t __tmp = (value);                                                   \
        __asm__ __volatile__("csrw " #reg ", %0" ::"r"(__tmp));                     \
    } while(0)

#define SATP_SV32   (1u << 31)
#define PAGE_V      (1 << 0)
#define PAGE_R      (1 << 1)
#define PAGE_W      (1 << 2)
#define PAGE_X      (1 << 3)
#define PAGE_U      (1 << 4)

#define PROCS_MAX       8
#define PROC_UNUSED     0
#define PROC_RUNNABLE   1
#define PROC_EXITED     2

#define SSTATUS_SPIE (1 << 5)

#define SCAUSE_ECALL 8

struct process {
    int                 pid;    
    int               state;    // (PROC_UNUSED | PROC_RUNNABLE)
    vaddr_t              sp;    // stack pointer
    uint32_t    *page_table;    // virtual mem addr -> physical mem addr 페이지 테이블
    uint8_t     stack[8192];    // kernel stack. 저장된 CPU 레지스터, 함수 리턴 주소, 로컬 변수 등으로 활용. (user stack과 다름)
};


struct trap_frame {
    uint32_t ra;
    uint32_t gp;
    uint32_t tp;
    uint32_t t0;
    uint32_t t1;
    uint32_t t2;
    uint32_t t3;
    uint32_t t4;
    uint32_t t5;
    uint32_t t6;
    uint32_t a0;
    uint32_t a1;
    uint32_t a2;
    uint32_t a3;
    uint32_t a4;
    uint32_t a5;
    uint32_t a6;
    uint32_t a7;
    uint32_t s0;
    uint32_t s1;
    uint32_t s2;
    uint32_t s3;
    uint32_t s4;
    uint32_t s5;
    uint32_t s6;
    uint32_t s7;
    uint32_t s8;
    uint32_t s9;
    uint32_t s10;
    uint32_t s11;
    uint32_t sp;
} __attribute__((packed));


struct sbiret {
    long error;
    long value;
};
