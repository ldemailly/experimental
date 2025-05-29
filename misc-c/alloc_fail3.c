// Disproving https://scvalex.net/posts/6/
// Just need, eg:
// ulimit -v 102400 # 100Mb
#include <stdio.h>
#include <stdlib.h>

int main(void) {
  printf("Before\n");
  int i = 0;
  int mb = 10;
  size_t sz = mb * 1024 * 1024;
  void *p;
  while (1) {
    p = malloc(sz);
    if (p == NULL) {
      perror("failed malloc");
      return 1;
    }
    printf("Allocated %d MB at %p\n", ++i*mb, p);
  }
  return 0;
}
