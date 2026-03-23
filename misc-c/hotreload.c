#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>


typedef int (*func_t)(int);

int myFunc1(int inp) {
    return 2*inp;
}

// read (could be from network, using a file to keep this simple)
char *read_data(char *fname) {
    FILE *f = fopen(fname, "rb");
    if (!f) {
        perror("fopen");
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

int main(void) {
    func_t myFunc = myFunc1;

    int inp;
    printf("input: ");
    scanf("%d", &inp);
    printf("myFunc1(%d) = %d\n", inp, myFunc(inp));
    char *data = read_data("data.bin");
    if (!data) {
        fprintf(stderr, "Failed to read data\n");
        return 1;
    }
    myFunc = (int (*)(int))data;
    printf("myFunc(%d) = %d\n", inp, myFunc(inp));
    
    return 0;
}
