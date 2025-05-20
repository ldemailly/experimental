#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define LIB_NAME "./libezreload.so"
#define MAX_RETRIES 5
#define RETRY_DELAY_US 200000 // 200ms

// Function pointer type for doWork
typedef void (*doWork_t)(void);

// Union for safe function pointer conversion
typedef union {
    void *obj;
    doWork_t func;
} func_ptr_union;

// Global variables
void *handle = NULL;
doWork_t doWork = NULL;

// Function to check if file has changed
int check_file_changed(const char *filename, time_t *last_modified) {
    struct stat st;
    if (stat(filename, &st) != 0) {
        printf("Failed to stat %s: %s\n", filename, strerror(errno));
        return 0;
    }
    if (st.st_mtime != *last_modified) {
        *last_modified = st.st_mtime;
        return 1;
    }
    return 0;
}

// Function to load the library or exit on failure.
void load_library(void) {
    void *new_handle;
    func_ptr_union fp;
    for (int retries = 0; retries < MAX_RETRIES; usleep(100000), retries++) {
        printf("Attempting to load library (attempt %d/%d)...\n", retries + 1, MAX_RETRIES);

        // First close old one
        if (handle) {
            printf("Closing old library handle %p\n", handle);
            if (dlclose(handle) != 0) {
                printf("Warning: Failed to close old library: %s\n", dlerror());
            }
            handle = NULL;
        }

        new_handle = dlopen(LIB_NAME, RTLD_NOW | RTLD_LOCAL);
        if (!new_handle) {
            printf("Failed to load library: %s\n", dlerror());
            continue;
        }
        fp.obj = dlsym(new_handle, "doWork");
        if (!fp.obj) {
            printf("Failed to find doWork: %s\n", dlerror());
            dlclose(new_handle);
            continue;
        }
        printf("Successfully loaded library, doWork %p\n", (void *)fp.obj);
        handle = new_handle;
        doWork = fp.func;
        return;
    }
    printf("Failed to load library after %d attempts\n", MAX_RETRIES);
    exit(1);
}

int main(void) {
    time_t last_modified = 0;
    // Main loop
    do {
        // (Re)load library if it has changed (ts 0 will reload at startup)
        if (check_file_changed(LIB_NAME, &last_modified)) {
            printf("Library file changed, (re)loading...\n");
            load_library();
        }
        // Call into the library (which will sleep inside)
        doWork();
    } while (1);

    // Cleanup (never reached in this example)
    if (handle) {
        dlclose(handle);
    }
    return 0;
}
