#include <dlfcn.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#define LIB_NAME "libezreload.so"
#define SLEEP_US 100000 // 100ms

typedef void (*doWorkFunc)(void);

time_t last_modified = 0;
void *lib_handle = NULL;
doWorkFunc doWork = NULL;
pthread_mutex_t doWork_mutex = PTHREAD_MUTEX_INITIALIZER;

void *open_library_and_resolve(void) {
    struct stat st;
    if (stat(LIB_NAME, &st) == 0) {
        last_modified = st.st_mtime;
    }

    // First try to unload the library if it's already loaded
    if (lib_handle != NULL) {
        printf("Closing old library handle %p\n", lib_handle);
        pthread_mutex_lock(&doWork_mutex);
        // Needed to not segfault during unload/reload.
        doWork = NULL; // Clear the function pointer before closing
        pthread_mutex_unlock(&doWork_mutex);
        if (dlclose(lib_handle) != 0) {
            dlerror();
            printf("Failed to close old library\n");
        }
        lib_handle = NULL;
    }

    // Now load the library fresh
    printf("Opening library\n");
    void *e = dlopen(LIB_NAME, RTLD_NOW | RTLD_LOCAL);
    if (e == NULL) {
        dlerror();
        printf("Failed to open %s\n", LIB_NAME);
        return NULL;
    }
    printf("Got new library handle %p\n", e);
    doWorkFunc new_doWork = (doWorkFunc)dlsym(e, "doWork");
    if (new_doWork == NULL) {
        dlerror();
        printf("Failed to find doWork\n");
        dlclose(e);
        return NULL;
    }
    printf("found new doWork: %p\n", (void *)new_doWork);

    pthread_mutex_lock(&doWork_mutex);
    doWork = new_doWork;
    pthread_mutex_unlock(&doWork_mutex);

    return e;
}

void *watch_library(void *arg __attribute__((unused))) {
    struct stat st;
    while (1) {
        if (stat(LIB_NAME, &st) != 0) {
            perror("Failed to stat libezreload.so");
            exit(1);
        }
        if (st.st_mtime > last_modified) {
            printf("Library file changed, reloading...\n");
            void *new_handle = open_library_and_resolve();
            if (new_handle == NULL) {
                printf("Failed to open library\n");
                exit(1);
            }
            lib_handle = new_handle;
            printf("Library reloaded\n");
        }
        usleep(SLEEP_US);
    }
}

int main(void) {
    lib_handle = open_library_and_resolve();
    if (lib_handle == NULL) {
        return 1;
    }

    pthread_t thread;
    pthread_create(&thread, NULL, watch_library, NULL);

    while (1) {
        pthread_mutex_lock(&doWork_mutex);
        doWorkFunc current_doWork = doWork;
        pthread_mutex_unlock(&doWork_mutex);

        if (current_doWork != NULL) {
            printf("doWork: %p\n", (void *)current_doWork);
            current_doWork();
        } else {
            printf("doWork is NULL, waiting for reload...\n");
        }
        usleep(SLEEP_US);
    }

    pthread_join(thread, NULL);
    return 0;
}
