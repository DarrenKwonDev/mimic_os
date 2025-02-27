ENTRY(boot)

/*
 * text   : 실행 가능한 코드가 저장된 영역 (함수)
 * data   : 초기화된 변경 가능한 전역 변수
 * rodata : 초기화된 변경 불가능한 전역 변수 (const, 문자열 리터럴)
 * bss    : 초기화되지 않은 전역 변수
 */

SECTIONS {
    /* base address .은 위치 카운터를 가리키는 예약어로, 실제 메모리 위치를 제어합니다. */
    . = 0x80200000;

    /* text 영역을 먼저 배치. 텍스트 이름을 wildcard 패턴으로 잡아 위치시킨다.  */
    .text :{
        KEEP(*(.text.boot));    /* 링커가 사용하지 않는 영역을 삭제하는 것을 방지 (KEEP)  */
        *(.text .text.*);       /* 입력 파일 중 .text 혹은 .text.* 란 이름을 가진 것을 현재 위치에 배치시킨다. */
    }
    /* 배치 후에는 위치 카운터가 자동으로 증가함 */

    /* 32bit OS이므로 4byte 정렬 */
    .rodata : ALIGN(4) {
        *(.rodata .rodata.*)    
    }

    .data : ALIGN(4) {
        *(.data .data.*);
    }

    .bss : ALIGN(4) {
        __bss = .;                      /* __bss 에 현재 메모리 위치(.) 저장 */
        *(.bss .bss.* .sbss .sbss.*);   /* sbss 는 small bss를 의미 */
        __bss_end = .;
    }

    . = ALIGN(4);
    . += 128 * 1024;    /* 현재 위치에 128KB 추가 할당 */
    __stack_top = .;    /* 스택의 최상단 주소를 __stack_top 변수에 저장 */

}
