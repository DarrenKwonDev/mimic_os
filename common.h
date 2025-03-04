#pragma once

typedef int                    bool;
typedef unsigned char       uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;

// x32 cpu를 가정하고 운영되므로
typedef uint32_t             size_t;
typedef uint32_t            paddr_t;
typedef uint32_t            vaddr_t;


#define SYS_PUTCHAR 1
#define SYS_EXIT    3

#define true    1
#define false   0
#define NULL    ((void *)0)

#define align_up(value, align)      __builtin_align_up(value, align)    // value를 align의 배수가 되도록 올림합니다
#define is_aligned(value, align)    __builtin_is_aligned(value, align)  // value가 align의 배수인지 확인합니다
#define offsetof(type, member)      __builtin_offsetof(type, member)

#define va_list     __builtin_va_list
#define va_start    __builtin_va_start
#define va_end      __builtin_va_end
#define va_arg      __builtin_va_arg

#define PAGE_SIZE       4096

void *memset(void *buf, char c, size_t n);
void *memcpy(void *dst, const void *src, size_t n);

void *strcpy(char *dst, const char *src);
int strcmp(const char *s1, const char *s2);


void printf(const char *fmt, ...);
