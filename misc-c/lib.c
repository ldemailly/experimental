#include <stdio.h>
#include <unistd.h>

const int version = 97;

int counter; // c99 guarantees initialization to zero

void doWork(void) {
    printf("Hello, inside call %d of doWork version %d!\n", counter, version);
    sleep(1);
    printf("Done with call %d of doWork version %d!\n", counter, version);
    counter++;
}
