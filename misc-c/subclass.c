#include <stdio.h>
#include <stdlib.h>

typedef struct {
    void (*doBar)();
} Foo;

void doBar() {
    printf("in bar\n");
}

Foo* NewBar() {
    Foo* b = (Foo*)malloc(sizeof(Foo));
    b->doBar = doBar;
    return b;
}

int main() {
    Foo *f = NewBar();
    f->doBar();
}