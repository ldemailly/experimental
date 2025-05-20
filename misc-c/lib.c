#include <stdio.h>
#include <unistd.h>

const int version = 66;

int counter;

void doWork(void) {
    printf("Hello, inside call %d of doWork version %d!\n", counter, version);
    sleep(1);
    printf("Done with call %d of doWork version %d!\n", counter, version);
    counter++;
}
