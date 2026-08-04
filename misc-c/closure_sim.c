// hand made closure.c equivalent.
#include <stdio.h>

typedef struct closure {
    int (*f)(int *bound);
    int *bound;
} closure_t;

#define call(c) ((c).f((c).bound))

void sister(closure_t c) {
    for (int i = 0; i < 10; i++) {
        printf("%d: %d\n", i, call(c));
    }
}

static int main_closure(int *bound) {
    return (*bound)++;
}

int main(void) {
    int bound = 42;
    closure_t c = { .f = main_closure, .bound = &bound };
    sister(c);
    printf("After %d\n", bound);
}
