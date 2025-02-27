typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef uint32_t size_t;

/* linker script에 작성한 것과 동일한 변수명이어야 함 */
extern char __bss[], __bss_end[], __stack_top[];

void *memset(void *buf, char c, size_t n)
{
    uint8_t *p = (uint8_t *)buf;
    while(n--)
        *p++ = c;
    return buf;
}

void kernel_main(void)
{
    // bss 영역을 0으로 초기화합니다.
    memset(__bss, 0, (size_t)__bss_end - (size_t)__bss);

    for (;;);
}

// 컴파일러에게 __attribute__를 통해 명령.
// section은 linker script에 등록한 섹션에 해당 함수를 배치하라.
// naked는 컴파일러가 함수에 대한 프롤로그(함수 시작 시 스택 프레임 설정 등)와
//                                에필로그(함수 종료 시 정리 작업) 코드를 생성하지 않음
__attribute__((section(".text.boot")))
__attribute__((naked))
void boot(void)
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
