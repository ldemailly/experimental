/*
 * Generates obfuscated short C program using the double representation of characters.
 * (not portable to endianess, but works on most modern platforms)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <string>\n", argv[0]);
        return 1;
    }
    const char *input = argv[1];
    size_t len = strlen(input);
    // translate to how many doubles we need:
    size_t num_doubles = (len + sizeof(double) - 1) / sizeof(double);
    double *doubles = (double *)calloc(num_doubles, sizeof(double));
    memcpy(doubles, input, len); // we could cast but let's be not UB
    puts("#include <stdio.h>");
    puts("int main(void) {");
    printf("  double s[%zu] = {", num_doubles + 1);
    for (size_t i = 0; i < num_doubles; i++) {
        printf("%a,", doubles[i]);
    }
    puts("0};\n  puts((char *)s);\n}");
    free(doubles);
    return 0;
}
