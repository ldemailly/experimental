#ifndef FILEMONITOR_H
#define FILEMONITOR_H

#include <pthread.h>

typedef void (*file_change_callback)(const char *filename, void *user_data);

struct monitor_args {
    const char *filename;
    file_change_callback callback;
    void *user_data;
};

void *monitor_file(void *arg);

#endif // FILEMONITOR_H
