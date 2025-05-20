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

// Global variables
void *handle = NULL;
doWork_t doWork = NULL;
pthread_mutex_t doWork_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t doWork_done = PTHREAD_COND_INITIALIZER;
int doWork_in_progress = 0;

// Function to load the library
void *load_library(void) {
    void *new_handle;
    doWork_t new_doWork;
    int *lib_version;
    int retries = 0;
    const int max_retries = 5;

    while (retries < max_retries) {
        printf("Attempting to load library (attempt %d/%d)...\n", retries + 1, max_retries);

        // Try to load the library with appropriate flags
#ifdef __APPLE__
        new_handle = dlopen(LIB_NAME, RTLD_NOW | RTLD_LOCAL | RTLD_NODELETE);
#elif defined(__linux__)
        new_handle = dlopen(LIB_NAME, RTLD_NOW | RTLD_LOCAL | RTLD_NODELETE);
#else
        new_handle = dlopen(LIB_NAME, RTLD_NOW | RTLD_LOCAL);
#endif

        if (!new_handle) {
            printf("Failed to load library: %s\n", dlerror());
            retries++;
            usleep(100000); // 100ms delay
            continue;
        }

        // Get the function pointer
        new_doWork = (doWork_t)dlsym(new_handle, "doWork");
        if (!new_doWork) {
            printf("Failed to find doWork: %s\n", dlerror());
            dlclose(new_handle);
            retries++;
            usleep(100000);
            continue;
        }

        // Get the version
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

    printf("\nLibrary file changed, reloading...\n");

    // Wait for any ongoing doWork to finish
    pthread_mutex_lock(&doWork_mutex);
    while (doWork_in_progress) {
        printf("Waiting for current doWork to finish...\n");
        pthread_cond_wait(&doWork_done, &doWork_mutex);
    }
    pthread_mutex_unlock(&doWork_mutex);

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

    // Close the old library
    if (handle) {
        dlclose(handle);
    }

    // Update the global variables
    handle = new_handle;
    doWork = new_doWork;
    printf("Library reloaded successfully, new version: %d\n", *lib_version);
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

    // Start file monitoring in a separate thread
    if (pthread_create(&monitor_thread, NULL, monitor_file, &args) != 0) {
        fprintf(stderr, "Failed to create monitor thread\n");
        dlclose(handle);
        return 1;
    }

    // Main loop
    while (1) {
        pthread_mutex_lock(&doWork_mutex);
        doWork_in_progress = 1;
        pthread_mutex_unlock(&doWork_mutex);

        doWork();

        pthread_mutex_lock(&doWork_mutex);
        doWork_in_progress = 0;
        pthread_cond_signal(&doWork_done);
        pthread_mutex_unlock(&doWork_mutex);

        sleep(1);
    }

    // Cleanup (never reached in this example)
    pthread_join(monitor_thread, NULL);
    dlclose(handle);
    return 0;
}
