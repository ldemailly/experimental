/*
  Works on macOS lenient utf-8 locale or on linux with:

  LC_ALL=en_US.ISO8859-1

  which can be created with:

  sudo localedef -i en_US -f ISO-8859-1 en_US.ISO-8859-1
*/
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    printf("LC_CTYPE before: %s\n", setlocale(LC_CTYPE, NULL));
    char *r = setlocale(LC_ALL, "en_US.ISO8859-1");
    if (r == NULL) {
        perror("setlocale");
        printf("run:\n\tsudo localedef -i en_US -f ISO-8859-1 en_US.ISO-8859-1\n");
        return 1;
    }
    printf("locale is now: %s\n", r);
    setenv("LC_ALL", "en_US.ISO8859-1", 1);
    char bad[] = {65, -1, 66, 0};
    printf("bad=%s\n", bad);
    int ret = setenv(bad, "example_val", 1);
    if (ret) {
        perror("setenv");
        return 1;
    }
    char *v = getenv(bad);
    printf("getenv = %s\n", v);
    char buf[128];
    snprintf(buf, sizeof(buf), "/bin/echo $%s", bad);
    char *shell = "/bin/bash";
    if (argc > 1) {
        shell = argv[1];
    }
    printf("will run '%s' in %s\n", buf, shell);
    execl(shell, shell, "-xe", "-c", buf, NULL);
    perror("execl"); // only reached if execl fails
}
