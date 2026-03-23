#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/errno.h>

typedef int (*func_t)(int);

int myFunc1(int inp) {
    return 2*inp;
}

// load new function/code (could be from network, using a file to keep this simple)
char *load_code(char *fname) {
    FILE *f = fopen(fname, "rb");
    if (!f) {
        fprintf(stderr, "fopen %s failed: %s\n", fname, strerror(errno));
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (fsize <= 0) {
        fclose(f);
        fprintf(stderr, "Invalid file size\n");
        return NULL;
    }
    char *data = mmap(NULL, (size_t)fsize, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (data == MAP_FAILED) {
        fclose(f);
        perror("mmap");
        return NULL;
    }
    size_t nread = fread(data, 1, (size_t)fsize, f);
    fclose(f);
    if (nread != (size_t)fsize) {
        munmap(data, (size_t)fsize);
        fprintf(stderr, "Short read\n");
        return NULL;
    }
    if (mprotect(data, (size_t)fsize, PROT_READ | PROT_EXEC) != 0) {
        munmap(data, (size_t)fsize);
        perror("mprotect");
        return NULL;
    }
    return data;
}

func_t change_func(char *fname) {
    char *data = load_code(fname);
    if (!data) {
        exit(1); // error already logged.
    }
    func_t result;
    // without -Wpedantic, we could just do: return (func_t)data;
    memcpy(&result, &data, sizeof(func_t));
    return result;
}

int main(void) {
    func_t myFunc = myFunc1;
    int inp;
    printf("input: ");
    scanf("%d", &inp);
    printf("before hot reload: myFunc(%d) = %d\n", inp, myFunc(inp));
    myFunc = change_func("code.bin");
    printf("after hot reload : myFunc(%d) = %d\n", inp, myFunc(inp));
    return 0;
}
