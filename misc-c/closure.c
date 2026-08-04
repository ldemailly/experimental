// gcc only : compile without -pedantic and possibly with  -ftrampoline-impl=heap
#include <stdio.h>

void sister(int (*f)()) {
    for (int i = 0; i < 10; i++) {
        printf("%d: %d\n", i, f());
    }
}

int main(void) {
    int bound = 42;
    int closure(void) {
        return bound++;
    }
    sister(closure);
    printf("After %d\n", bound);
}
