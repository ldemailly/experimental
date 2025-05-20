#include <dlfcn.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#ifdef __APPLE__
#include <sys/event.h>
#elif defined(__linux__)
#include <sys/inotify.h>
#endif

#define LIB_NAME "./libezreload.so"
#define SLEEP_US 100000 // 100ms
#define MAX_RETRIES 5
#define RETRY_DELAY_US 200000 // 200ms

typedef void (*doWorkFunc)(void);

// Union to handle function pointer conversions in a standard-compliant way
typedef union {
    doWorkFunc func;
    void *ptr;
} func_ptr_union;

time_t last_modified = 0;
void *lib_handle = NULL;
doWorkFunc doWork = NULL;
pthread_mutex_t doWork_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t doWork_done = PTHREAD_COND_INITIALIZER;
int doWork_in_progress = 0;

void *open_library_and_resolve(void) {
    struct stat st;
    int retries = 0;

    // Try to stat the file with retries
    while (stat(LIB_NAME, &st) != 0) {
        if (retries++ >= MAX_RETRIES) {
            printf("Failed to stat %s after %d retries: %s\n", LIB_NAME, MAX_RETRIES, strerror(errno));
            return NULL;
        }
        printf("Library file temporarily not found, retrying (%d/%d)...\n", retries, MAX_RETRIES);
        usleep(RETRY_DELAY_US);
    }

    printf("Library file exists, size: %ld bytes\n", (long)st.st_size);
    last_modified = st.st_mtime;

    // Wait for any ongoing doWork to finish
    pthread_mutex_lock(&doWork_mutex);
    while (doWork_in_progress) {
        printf("Waiting for current doWork to finish...\n");
        pthread_cond_wait(&doWork_done, &doWork_mutex);
    }

    // First try to unload the library if it's already loaded
    if (lib_handle != NULL) {
        printf("Closing old library handle %p\n", lib_handle);
        // Needed to not segfault during unload/reload.
        doWork = NULL; // Clear the function pointer before closing
        if (dlclose(lib_handle) != 0) {
            printf("Failed to close old library: %s\n", dlerror());
        }
        lib_handle = NULL;
    }
    pthread_mutex_unlock(&doWork_mutex);

    // Now load the library fresh with retries
    retries = 0;
    void *e = NULL;
    while (retries < MAX_RETRIES) {
        printf("Opening library %s (attempt %d/%d)\n", LIB_NAME, retries + 1, MAX_RETRIES);
        e = dlopen(LIB_NAME, RTLD_NOW | RTLD_LOCAL);
        if (e != NULL) break;

        char *err = dlerror();
        printf("Failed to open %s: %s\n", LIB_NAME, err ? err : "unknown error");
        retries++;
        if (retries < MAX_RETRIES) {
            printf("Retrying in %d ms...\n", RETRY_DELAY_US/1000);
            usleep(RETRY_DELAY_US);
        }
    }

    if (e == NULL) {
        printf("Failed to open library after %d attempts\n", MAX_RETRIES);
        return NULL;
    }

    printf("Got new library handle %p\n", e);

    func_ptr_union u;
    u.ptr = dlsym(e, "doWork");
    if (u.ptr == NULL) {
        char *err = dlerror();
        printf("Failed to find doWork: %s\n", err ? err : "unknown error");
        dlclose(e);
        return NULL;
    }
    printf("found new doWork: %p (library base: %p)\n", u.ptr, e);

    // Get the version from the library
    int *lib_version = dlsym(e, "version");
    if (lib_version != NULL) {
        printf("Library version: %d\n", *lib_version);
    }

    pthread_mutex_lock(&doWork_mutex);
    doWork = u.func;
    pthread_mutex_unlock(&doWork_mutex);

    return e;
}

void *watch_library(void *arg __attribute__((unused))) {
#ifdef __APPLE__
    int kq = kqueue();
    if (kq == -1) {
        perror("kqueue");
        exit(1);
    }

    int fd = open(LIB_NAME, O_RDONLY);
    if (fd == -1) {
        perror("open");
        exit(1);
    }

    struct kevent event;
    EV_SET(&event, fd, EVFILT_VNODE, EV_ADD | EV_ENABLE | EV_CLEAR,
           NOTE_DELETE | NOTE_WRITE | NOTE_EXTEND | NOTE_ATTRIB | NOTE_LINK | NOTE_RENAME | NOTE_REVOKE,
           0, NULL);

    if (kevent(kq, &event, 1, NULL, 0, NULL) == -1) {
        perror("kevent");
        exit(1);
    }

    while (1) {
        struct kevent event;
        if (kevent(kq, NULL, 0, &event, 1, NULL) == -1) {
            perror("kevent");
            exit(1);
        }

        if (event.ident == (uintptr_t)fd) {
            printf("Library file changed, reloading...\n");
            void *new_handle = open_library_and_resolve();
            if (new_handle == NULL) {
                printf("Failed to open library\n");
                exit(1);
            }
            lib_handle = new_handle;
            printf("Library reloaded\n");
        }
    }
#elif defined(__linux__)
    int fd = inotify_init();
    if (fd == -1) {
        perror("inotify_init");
        exit(1);
    }

    // Watch for more events that might occur during file modification
    int wd = inotify_add_watch(fd, LIB_NAME,
        IN_MODIFY | IN_CLOSE_WRITE | IN_DELETE_SELF | IN_MOVE_SELF | IN_ATTRIB);
    if (wd == -1) {
        perror("inotify_add_watch");
        exit(1);
    }

    while (1) {
        char buf[4096] __attribute__((aligned(8)));
        int len = read(fd, buf, sizeof(buf));
        if (len == -1) {
            perror("read");
            exit(1);
        }

        for (char *ptr = buf; ptr < buf + len;) {
            struct inotify_event *event = (struct inotify_event *)ptr;
            printf("inotify event: mask=0x%x\n", event->mask);

            if (event->mask & (IN_MODIFY | IN_CLOSE_WRITE | IN_DELETE_SELF | IN_MOVE_SELF | IN_ATTRIB)) {
                printf("Library file changed (event mask: 0x%x), reloading...\n", event->mask);
                void *new_handle = open_library_and_resolve();
                if (new_handle == NULL) {
                    printf("Failed to open library\n");
                    exit(1);
                }
                lib_handle = new_handle;
                printf("Library reloaded\n");
            }
            ptr += sizeof(struct inotify_event) + event->len;
        }
    }
#else
#error "Unsupported platform"
#endif
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
        if (current_doWork != NULL) {
            doWork_in_progress = 1;
            pthread_mutex_unlock(&doWork_mutex);

            func_ptr_union u;
            u.func = current_doWork;
            printf("doWork: %p\n", u.ptr);
            current_doWork();

            pthread_mutex_lock(&doWork_mutex);
            doWork_in_progress = 0;
            pthread_cond_signal(&doWork_done);
            pthread_mutex_unlock(&doWork_mutex);
        } else {
            pthread_mutex_unlock(&doWork_mutex);
            printf("doWork is NULL, waiting for reload...\n");
            usleep(SLEEP_US);
        }
    }

    pthread_join(thread, NULL);
    return 0;
}
