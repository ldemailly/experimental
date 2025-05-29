// Get alloc failure ahead of kill when using mlock (and ulimit -l)
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

int main(void) {
  printf("Before\n");
  int i = 0;
  int err;
  size_t sz = 100 * 1024 * 1024;
  void *p;
  while (1) {
    p = mmap(NULL, sz, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
    if (errno != 0) {
      perror("failed alloc");
      return 1;
    }
    printf("Allocated %d MB at %p\n", ++i*100, p);
    err = mlock(p, sz);
    if (err != 0) {
      perror("mlock failed");
    }
    memset(p, sz, 43);
  }
  printf("After\n");
  return 0;
}
