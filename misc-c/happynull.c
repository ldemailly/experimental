#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>


// Works on Linux with
// echo 0 > /proc/sys/vm/mmap_min_addr
// init function that runs before main()
__attribute__((constructor))
void pagingDrZero(void) {
  void *start = (void *)0;
  void *p = mmap(start, 4096, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_ANON | MAP_PRIVATE, -1, 0);
  if (errno != 0) {
      perror("failed alloc did you echo 0 > /proc/sys/vm/mmap_min_addr ?");
      exit(1);
  }
  if (p) {
    printf("Allocated page at %p\n", p);
  }
}

int main(void) {
  int *p = NULL;
  printf("we're going to read and write to pointer: %p (and not crash)\n", (void*)p);
  int x;
  x = *p; // read from NULL pointer
  printf("%d\n", x);
  *p = 42; // write to NULL pointer
  printf("%d\n", *p);
  return 0;
}
