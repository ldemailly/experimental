#include <unistd.h>
static void x() {
  static const char str[] = "Hello World!\n";
  write(1,str,sizeof(str));
}
int main() {x(); return 42;}
