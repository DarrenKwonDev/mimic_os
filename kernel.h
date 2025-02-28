#pragma once

// do-while(0) 패턴. 
// 세미콜론 문제를 해결하고 매크로 내 break, continue가 외부 루프에 주는 영향을 방지합니다
// 여러 줄로 된 macro를 작성할 때는 관습적으로 작성함  
#define PANIC(fmt, ...)                                                             \
    do {                                                                            \
        printf("PANIC: %s:%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__);       \
        while (1) {}                                                                \
    } while(0)




struct sbiret {
    long error;
    long value;
};
