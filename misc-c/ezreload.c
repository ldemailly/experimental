#include <dlfcn.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include "filemonitor.h"

#ifdef __APPLE__
#include <sys/event.h>
#elif defined(__linux__)
#include <sys/inotify.h>
#endif

#define LIB_NAME "./libezreload.so"
#define SLEEP_US 100000 // 100ms
#define MAX_RETRIES 5
#define RETRY_DELAY_US 200000 // 200ms

// Function pointer type for doWork
typedef void (*doWork_t)(void);

// Library state structure
struct lib_state {
    void *handle;
    doWork_t doWork;
    int ref_count;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
};

// Global variables
struct lib_state lib = {
    .handle = NULL,
    .doWork = NULL,
    .ref_count = 0,
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .cond = PTHREAD_COND_INITIALIZER
};

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

// Function to acquire library reference
void acquire_lib(void) {
    pthread_mutex_lock(&lib.mutex);
    lib.ref_count++;
    pthread_mutex_unlock(&lib.mutex);
}

// Function to release library reference
void release_lib(void) {
    pthread_mutex_lock(&lib.mutex);
    lib.ref_count--;
    if (lib.ref_count == 0) {
        pthread_cond_signal(&lib.cond);
    }
    pthread_mutex_unlock(&lib.mutex);
}

// Function to reload the library
void reload_library(void) {
    void *new_handle;
    doWork_t new_doWork;
    int *lib_version;

    printf("\nLibrary file changed, reloading...\n");

    // Wait for all library operations to complete
    pthread_mutex_lock(&lib.mutex);
    while (lib.ref_count > 0) {
        printf("Waiting for %d library operations to complete...\n", lib.ref_count);
        pthread_cond_wait(&lib.cond, &lib.mutex);
    }
    pthread_mutex_unlock(&lib.mutex);

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
    void *old_handle = lib.handle;

    // Update the library state
    pthread_mutex_lock(&lib.mutex);
    lib.handle = new_handle;
    lib.doWork = new_doWork;
    pthread_mutex_unlock(&lib.mutex);

    printf("Library reloaded successfully, new version: %d\n", *lib_version);

    // Close the old library
    if (old_handle) {
        printf("Closing old library handle %p\n", old_handle);
        if (dlclose(old_handle) != 0) {
            printf("Warning: Failed to close old library: %s\n", dlerror());
        }
    }
}

// Callback for file changes
void on_file_change(const char *filename, void *user_data) {
    (void)user_data;  // Silence unused parameter warning
    printf("File change detected: %s\n", filename);
    reload_library();
}

int main(void) {
    pthread_t monitor_thread;
    struct monitor_args args = {
        .filename = LIB_NAME,
        .callback = on_file_change,
        .user_data = NULL
    };

    // Initial library load
    lib.handle = load_library();
    if (!lib.handle) {
        fprintf(stderr, "Failed to load library: %s\n", dlerror());
        return 1;
    }

    // Get initial function pointer and version
    lib.doWork = (doWork_t)dlsym(lib.handle, "doWork");
    int *lib_version = (int *)dlsym(lib.handle, "version");
    if (!lib.doWork || !lib_version) {
        fprintf(stderr, "Failed to get symbols: %s\n", dlerror());
        dlclose(lib.handle);
        return 1;
    }

    printf("Initial library version: %d\n", *lib_version);
    printf("Initial doWork function at: %p\n", (void *)lib.doWork);

    // Start file monitoring in a separate thread
    if (pthread_create(&monitor_thread, NULL, monitor_file, &args) != 0) {
        fprintf(stderr, "Failed to create monitor thread\n");
        dlclose(lib.handle);
        return 1;
    }

    // Main loop
    while (1) {
        acquire_lib();

        // Get current function pointer under lock
        pthread_mutex_lock(&lib.mutex);
        doWork_t current_doWork = lib.doWork;
        pthread_mutex_unlock(&lib.mutex);

        if (current_doWork) {
            current_doWork();
        }

        release_lib();
        sleep(1);
    }

    // Cleanup (never reached in this example)
    pthread_join(monitor_thread, NULL);
    dlclose(lib.handle);
    return 0;
}
