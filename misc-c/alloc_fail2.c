// Get killed by using memory (memset) past a cgroup limit
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

int main(void) {
  printf("Before\n");
  int i = 0;
  size_t sz = 100 * 1024 * 1024;
  void *p;
  while (1) {
    p = mmap(NULL, sz, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
    if (errno != 0) {
      perror("failed alloc");
      return 1;
    }
    printf("Allocated %d MB at %p\n", ++i*100, p);
    memset(p, 43, sz);
  }
  printf("After\n");
  return 0;
}
