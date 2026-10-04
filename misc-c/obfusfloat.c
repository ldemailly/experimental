/*
 * Generates obfuscated short C program using the float representation of characters.
 * copied/inspired from obfusdbl.c
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
    // translate to how many floats we need:
    // (and include room for trailing \0 in the last float)
    size_t num_floats = (len + 1 + sizeof(float) - 1) / sizeof(float);
    float *floats = (float *)calloc(num_floats, sizeof(float));
    memcpy(floats, input, len); // we could cast but let's be not UB
    puts("#include <stdio.h>");
    puts("int main(void) {");
    if (num_floats == 1) {
        printf("  float d = %af;\n", floats[0]);
        puts("  puts((char *)&d);\n}");
    } else {
        printf("  float s[%zu] = {", num_floats);
        for (size_t i = 0; i < num_floats; i++) {
            printf("%af%s", floats[i], i + 1 == num_floats ? "" : ",");
        }
        puts("};\n  puts((char *)s);\n}");
    }
    free(floats);
    return 0;
}
