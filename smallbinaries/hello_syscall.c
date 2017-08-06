#include <sys/syscall.h>

static const char str[] = "Hello World!\n";

void _start() {
	syscall(SYS_write, 1, str, sizeof(str) - 1);
	syscall(SYS_exit, 42);
}
