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

    if (*last_modified == 0) {
        *last_modified = st.st_mtime;
        return 0;
    }

    if (st.st_mtime != *last_modified) {
        *last_modified = st.st_mtime;
        return 1;
    }

    return 0;
}

// Function to load the library
void *load_library(void) {
    void *new_handle;
    doWork_t new_doWork;
    int *lib_version;
    int retries = 0;
    const int max_retries = 5;

    while (retries < max_retries) {
        printf("Attempting to load library (attempt %d/%d)...\n", retries + 1, max_retries);

        new_handle = dlopen(LIB_NAME, RTLD_NOW | RTLD_LOCAL);
        if (!new_handle) {
            printf("Failed to load library: %s\n", dlerror());
            retries++;
            usleep(100000);
            continue;
        }

        new_doWork = (doWork_t)dlsym(new_handle, "doWork");
        if (!new_doWork) {
            printf("Failed to find doWork: %s\n", dlerror());
            dlclose(new_handle);
            retries++;
            usleep(100000);
            continue;
        }

        lib_version = (int *)dlsym(new_handle, "version");
        if (!lib_version) {
            printf("Failed to find version: %s\n", dlerror());
            dlclose(new_handle);
            retries++;
            usleep(100000);
            continue;
        }

        printf("Successfully loaded library version %d\n", *lib_version);
        printf("doWork function at: %p\n", (void *)new_doWork);
        return new_handle;
    }

    printf("Failed to load library after %d attempts\n", max_retries);
    return NULL;
}

// Function to reload the library
void reload_library(void) {
    void *new_handle;
    doWork_t new_doWork;
    int *lib_version;

    printf("Library file changed, reloading...\n");

    // Load the new library
    new_handle = load_library();
    if (!new_handle) {
        printf("Failed to load new library, keeping old one\n");
        return;
    }

    // Get the new function pointer and version
    new_doWork = (doWork_t)dlsym(new_handle, "doWork");
    lib_version = (int *)dlsym(new_handle, "version");

    if (!new_doWork || !lib_version) {
        printf("Failed to get new symbols: %s\n", dlerror());
        dlclose(new_handle);
        return;
    }

    // Store old handle for cleanup
    void *old_handle = handle;

    // Update the global variables
    handle = new_handle;
    doWork = new_doWork;
    printf("Library reloaded successfully, new version: %d\n", *lib_version);

    // Close the old library
    if (old_handle) {
        printf("Closing old library handle %p\n", old_handle);
        if (dlclose(old_handle) != 0) {
            printf("Warning: Failed to close old library: %s\n", dlerror());
        }
    }
}

int main(void) {
    time_t last_modified = 0;

    // Initial library load
    handle = load_library();
    if (!handle) {
        fprintf(stderr, "Failed to load library: %s\n", dlerror());
        return 1;
    }

    // Get initial function pointer and version
    doWork = (doWork_t)dlsym(handle, "doWork");
    int *lib_version = (int *)dlsym(handle, "version");
    if (!doWork || !lib_version) {
        fprintf(stderr, "Failed to get symbols: %s\n", dlerror());
        dlclose(handle);
        return 1;
    }

    printf("Initial library version: %d\n", *lib_version);
    printf("Initial doWork function at: %p\n", (void *)doWork);

    // Main loop
    while (1) {
        // Call doWork
        doWork();

        // Check if library file has changed
        if (check_file_changed(LIB_NAME, &last_modified)) {
            reload_library();
        }

        sleep(1);
    }

    // Cleanup (never reached in this example)
    dlclose(handle);
    return 0;
}
