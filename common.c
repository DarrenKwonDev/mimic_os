#include "common.h"

//-----------------------------------------
// 전방 선언
void putchar(char ch);


//----------------------------------------
// 함수 정의

// buf fill c, n bytes
void *memset(void *buf, char c, size_t n)
{
    uint8_t *p = (uint8_t *)buf;
    while(n--)
        *p++ = c;
    return buf;
}


// src to dst copy, n bytes
void *memcpy(void *dst, const void *src, size_t n)
{
    uint8_t *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;
    while(n--)
        *d++ = *s++;
    return dst;
}

// copy src to dst
void *strcpy(char *dst, const char *src)
{
    char *d = dst;
    while (*src)
        *d++ = *src++;
    *d = '\0';
    return dst;
}

// cmp s1 - s2 > 0, lexicographical order. e.g. 'a' > 'b'
int strcmp(const char *s1, const char *s2)
{
    while (*s1 && *s2)
    {
        if (*s1 != *s2)
            break;
        s1++;
        s2++;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

void printf(const char *fmt, ...)
{
    va_list vargs;
    va_start(vargs, fmt);

    while(*fmt)
    {
        if (*fmt == '%')
        {
            fmt++; // skip %

            switch (*fmt)
            {
                case '\0':
                    {
                        putchar('%');
                        goto end;
                    }

                case '%':
                    {
                        putchar('%');
                        break;
                    }

                case 's':
                    {
                        const char *s = va_arg(vargs, const char *);
                        while (*s)
                        {
                            putchar(*s);
                            s++;
                        }
                        break;
                    }

                case 'd':
                    {
                        /*
                           INT_MAX는 2,147,483,647 (0x7FFFFFFF)
                           INT_MIN은 -2,147,483,648 (0x80000000)

                           단순히 value = -value 꼴로 변경한다면
                           INT_MIN의 경우 2,147,483,648 이 되는데 이는 INT_MAX를 넘긴 값이 됨.

                           unsigned를 통해서 처리해야 바른 값을 표현할 수 있음
                        */
                        int value = va_arg(vargs, int);
                        unsigned magnitude = value;

                        if (value < 0)
                        {
                            putchar('-');
                            magnitude = -magnitude;
                        }

                        // 가장 큰 자릿수를 찾습니다
                        unsigned divisor = 1;
                        while(magnitude / divisor > 9)
                            divisor *= 10;

                        // 큰 자릿수부터 순차적으로 출력
                        while(divisor > 0)
                        {
                            putchar('0' + magnitude / divisor); // turn number to char
                            magnitude %= divisor;
                            divisor /= 10;
                        }

                        break;
                    }

                case 'x':
                    {
                        /*
                           4bit(nibble) 씩 옮겨서 16진수로 만듦
                        */
                        unsigned value = va_arg(vargs, unsigned);
                        for (int i = 7; i >= 0; i--)
                        {
                            unsigned nibble = (value >> (i * 4)) & 0xf;
                            putchar("0123456789abcdef"[nibble]);
                        }
                    }

            }
        }
        else
        {
            putchar(*fmt);
        }

        fmt++;

    }


end:
    va_end(vargs);
}
