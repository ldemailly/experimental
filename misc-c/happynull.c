// Works on Linux with
// echo 0 > /proc/sys/vm/mmap_min_addr
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

// The trick part:
// init function that runs before main(), just to be
// able to share main()'s full body for "quizz" purposes,
// otherwise we'd just call it explicitly from main().
__attribute__((constructor))
static void pagingDrZero(void) {
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

// The fun part:
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
