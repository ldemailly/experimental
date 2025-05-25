#include <stdio.h>

// packed struct of 3 int and array of 3 ints are the same
typedef struct __attribute__((packed, aligned(sizeof(int)))) {
    int a,b,c;
} t3;

void set(int v[], int n) {
    for (int i = 0; i < n; i++) {
        v[i] = 42+i;
    }
}

int main(void) {
    t3 t;
    t.a = -1;
    t.b = -2;
    t.c = -3;
    fprintf(stdout, "sizeof(t3): %lu vs sizeof(int): %lu\n", sizeof(t), sizeof(int));
    set(&t.b, 2);
    fprintf(stdout, "a: %d b: %d c: %d\n", t.a, t.b, t.c);
    return 0;
}
