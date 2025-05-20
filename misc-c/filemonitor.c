#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

#ifdef __APPLE__
#include <sys/event.h>
#include <fcntl.h>
#elif defined(__linux__)
#include <sys/inotify.h>
#endif

#include "filemonitor.h"

#define MAX_RETRIES 5
#define RETRY_DELAY_US 200000 // 200ms

static int check_file_ready(const char *filename) {
    struct stat st;
    int retries = 0;

    while (stat(filename, &st) != 0 || st.st_size == 0) {
        if (retries++ >= MAX_RETRIES) {
            printf("File %s not ready after %d retries\n", filename, MAX_RETRIES);
            return 0;
        }
        printf("File %s not ready (size: %lld), retrying...\n", filename, (long long)st.st_size);
        usleep(RETRY_DELAY_US);
    }

    printf("File %s is ready (size: %lld)\n", filename, (long long)st.st_size);
    return 1;
}

#ifdef __APPLE__
void *monitor_file(void *arg) {
    struct monitor_args *args = (struct monitor_args *)arg;
    int kq = kqueue();
    if (kq == -1) {
        perror("kqueue");
        return NULL;
    }

    int fd = open(args->filename, O_RDONLY);
    if (fd == -1) {
        perror("open");
        return NULL;
    }

    struct kevent event;
    EV_SET(&event, fd, EVFILT_VNODE, EV_ADD | EV_ENABLE | EV_CLEAR,
           NOTE_DELETE | NOTE_WRITE | NOTE_EXTEND | NOTE_ATTRIB | NOTE_LINK | NOTE_RENAME | NOTE_REVOKE,
           0, NULL);

    if (kevent(kq, &event, 1, NULL, 0, NULL) == -1) {
        perror("kevent");
        close(fd);
        return NULL;
    }

    while (1) {
        struct kevent event;
        if (kevent(kq, NULL, 0, &event, 1, NULL) == -1) {
            perror("kevent");
            close(fd);
            return NULL;
        }

        if (event.ident == (uintptr_t)fd) {
            if (event.fflags & (NOTE_DELETE | NOTE_RENAME)) {
                printf("File %s deleted or renamed, re-establishing watch...\n", args->filename);
                close(fd);
                // Wait for file to reappear
                while ((fd = open(args->filename, O_RDONLY)) == -1) {
                    usleep(RETRY_DELAY_US);
                }
                EV_SET(&event, fd, EVFILT_VNODE, EV_ADD | EV_ENABLE | EV_CLEAR,
                       NOTE_DELETE | NOTE_WRITE | NOTE_EXTEND | NOTE_ATTRIB | NOTE_LINK | NOTE_RENAME | NOTE_REVOKE,
                       0, NULL);
                if (kevent(kq, &event, 1, NULL, 0, NULL) == -1) {
                    perror("kevent (re-add)");
                    close(fd);
                    return NULL;
                }
                printf("Watch re-established for %s\n", args->filename);
                // Immediately check and call callback if file is ready
                if (check_file_ready(args->filename)) {
                    args->callback(args->filename, args->user_data);
                }
                continue;
            }
            printf("File %s changed, checking if ready...\n", args->filename);
            if (check_file_ready(args->filename)) {
                args->callback(args->filename, args->user_data);
            }
        }
    }
}
#elif defined(__linux__)
void *monitor_file(void *arg) {
    struct monitor_args *args = (struct monitor_args *)arg;
    int fd = inotify_init();
    if (fd == -1) {
        perror("inotify_init");
        return NULL;
    }

    int wd = inotify_add_watch(fd, args->filename,
        IN_MODIFY | IN_CLOSE_WRITE | IN_DELETE_SELF | IN_MOVE_SELF | IN_ATTRIB);
    if (wd == -1) {
        perror("inotify_add_watch");
        close(fd);
        return NULL;
    }

    while (1) {
        char buf[4096] __attribute__((aligned(8)));
        int len = read(fd, buf, sizeof(buf));
        if (len == -1) {
            perror("read");
            close(fd);
            return NULL;
        }

        for (char *ptr = buf; ptr < buf + len;) {
            struct inotify_event *event = (struct inotify_event *)ptr;
            printf("inotify event: mask=0x%x\n", event->mask);

            if (event->mask & (IN_DELETE_SELF | IN_MOVE_SELF)) {
                printf("File %s deleted or moved, re-establishing watch...\n", args->filename);
                inotify_rm_watch(fd, wd);
                // Wait for file to reappear
                struct stat st;
                while (stat(args->filename, &st) != 0) {
                    usleep(RETRY_DELAY_US);
                }
                wd = inotify_add_watch(fd, args->filename,
                    IN_MODIFY | IN_CLOSE_WRITE | IN_DELETE_SELF | IN_MOVE_SELF | IN_ATTRIB);
                if (wd == -1) {
                    perror("inotify_add_watch (re-add)");
                    close(fd);
                    return NULL;
                }
                printf("Watch re-established for %s\n", args->filename);
                // Immediately check and call callback if file is ready
                if (check_file_ready(args->filename)) {
                    args->callback(args->filename, args->user_data);
                }
                ptr += sizeof(struct inotify_event) + event->len;
                continue;
            }

            if (event->mask & (IN_MODIFY | IN_CLOSE_WRITE | IN_ATTRIB)) {
                printf("File %s changed, checking if ready...\n", args->filename);
                if (check_file_ready(args->filename)) {
                    args->callback(args->filename, args->user_data);
                }
            }
            ptr += sizeof(struct inotify_event) + event->len;
        }
    }
}
#else
#error "Unsupported platform"
#endif
