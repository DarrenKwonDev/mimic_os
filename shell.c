#include "user.h"

void main(void)
{
    // *((volatile int *) 0x80200000) = 0x1234; // should panic
    

    printf("Hello world from user level\n");

    exit();

    // for(;;);
}
