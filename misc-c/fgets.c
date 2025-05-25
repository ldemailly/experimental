#include <stdio.h>

// union of 3 bytes and array of 3 bytes
typedef struct __attribute__((packed)) {
    char c,b,a;
} t3;

typedef union {
    t3 t;
    char x[3];
} u3;

int main(void) {
     // 'correct' version:
    t3 t; // same as u3.t
    t.a = 1;
    t.b = 2;
    t.c = 3;
    fprintf(stdout, "sizeof(t3): %lu\n", sizeof(t));
    fgets(&t.b, 2, stdin);
    fprintf(stdout, "a: %d b: %d c: %d\n", t.a, t.b, t.c);
    // yet same on the stack directly: UB...ish
    char a,b,c;
    a = 1;
    b = 2;
    c = 3;
    fgets(&b, 2, stdin);
    fprintf(stdout, "a: %d b: %d c: %d\n", a, b, c);
    return 0;
}
