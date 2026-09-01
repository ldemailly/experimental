// Compile with
// clang -std=c23 -Wall -Wextra -pedantic -Werror -O3  collatz_opt.c -march=native -pthread -o collatz_opt
// See collatz.c for a simpler version that uses always 128bits and is almost 2x slower.
// Note that on apple silicon clang is almost 3.2x faster than gcc 16.2.0 somehow.

#include <errno.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BITSIZE 128
#define NUM_THREADS 8
// Odd numbers per work chunk; results are buffered per chunk so memory stays bounded
// while still being printed/reduced strictly in order once the chunk is done.
constexpr size_t chunk_size = NUM_THREADS * (1 << 17);
// Contiguous granule handed out per work-stealing grab: big enough to avoid false
// sharing between threads (cache lines) and to amortize atomic contention, small
// enough that a thread landing on a run of "hard" numbers doesn't stall the others.
constexpr size_t granule_size = 256;

bool atobigi(const char *str, _BitInt(BITSIZE) * out_val) {
    if (str == NULL || *str == '\0')
        return false;
    // to handle -170141183460469231731687303715884105728 correctly.
    unsigned _BitInt(BITSIZE) value = 0;
    bool is_negative = false;
    if (*str == '-') {
        is_negative = true;
        str++;
    } else if (*str == '+') {
        str++;
    }
    if (*str == '\0')
        return false; // missing digits after sign
    for (; *str != '\0'; str++) {
        char c = *str;
        if (c == '_' || c == ' ' || c == '\'') {
            continue; // allow ' _ and space as digit separators
        }
        if (c < '0' || c > '9') {
            return false; // Found a non-numeric character
        }
        value = (value * 10wb) + (c - '0');
    }
    if (is_negative) {
        *out_val = (_BitInt(BITSIZE))(-value);
    } else {
        *out_val = (_BitInt(BITSIZE))value;
    }
    return true;
}

void print(_BitInt(BITSIZE) value) {
    // buffer to hold decimal representation of the number based on
    // log of 10 base 2, plus one for sign and one for null terminator
    // 146/485 is slightly above the 0.3010299... of the log10(2).
    constexpr int digits = (BITSIZE * 146) / 485 + 1;
    // adding ' every 3 digits for readability.
    constexpr int len =
        digits + (digits - 1) / 3 + 2; // +1 for sign, +1 for null terminator, +1 for safety
    char buffer[len];
    buffer[len - 1] = '\0';
    int i = len - 2;
    int sign = (value < 0wb) ? -1 : 1;
    int n = 0;
    do {
        if (n > 0 && (n % 3) == 0) {
            buffer[i--] = '\'';
        }
        buffer[i--] = '0' + sign * (value % 10wb);
        value /= 10wb;
        n++;
    } while (value != 0wb);
    if (sign == -1) {
        buffer[i--] = '-';
    }
    fputs(buffer + i + 1, stdout);
}

constexpr _BitInt(BITSIZE) maxcollatz = ((_BitInt(BITSIZE))((~(unsigned _BitInt(BITSIZE))0) >> 1) -
                                         1wb) /
                                        3wb;

// Largest n for which 3*n+1 still fits in a uint64_t.
constexpr uint64_t maxcollatz64 = (UINT64_MAX - 1) / 3;

// Returns the max reached. -negative if overflow would occur.
// Odd n only: 3n+1 is always the peak of a run (the halving chain that
// follows is strictly decreasing), so we fast-forward straight to the next
// odd value in one step instead of halving one bit at a time.
_BitInt(BITSIZE) collatzrun(_BitInt(BITSIZE) n) {
    _BitInt(BITSIZE) max = n;
    // as we run everything in sequence, as soon as we dip below previous
    // value, we're done.
    _BitInt(BITSIZE) prev = n - 1;
    for (;;) {
        if (n > maxcollatz) {
            fprintf(stderr, "Overflow would occur during Collatz computation.\n");
            return -max; // return negative max to indicate overflow
        }
        n = 3wb * n + 1wb;
        if (n > max) {
            max = n;
        }
        n >>= __builtin_ctzg((unsigned _BitInt(BITSIZE))n);
        if (n <= prev) {
            return max;
        }
    }
}

// Same as collatzrun() but in native 64-bit arithmetic; returns false if n
// would grow past maxcollatz64, in which case the caller must fall back to
// the 128-bit collatzrun().
bool collatzrun64(uint64_t n, uint64_t *max_out) {
    uint64_t max = n;
    uint64_t prev = n - 1;
    for (;;) {
        if (n > maxcollatz64) {
            return false;
        }
        n = 3 * n + 1;
        if (n > max) {
            max = n;
        }
        n >>= __builtin_ctzll(n);
        if (n <= prev) {
            *max_out = max;
            return true;
        }
    }
}

// Persistent thread pool state, shared by all NUM_THREADS workers for the whole run:
// avoids the syscall overhead of pthread_create/join on every chunk. Workers block on
// cv_start between chunks and the main thread blocks on cv_done while a chunk runs.
typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t cv_start; // signaled by main when a new chunk (or shutdown) is ready
    pthread_cond_t cv_done;  // signaled by the last worker to finish the current chunk
    int generation;          // bumped by main for each new chunk, workers wait for a change
    int workers_left;        // workers still processing the current chunk
    bool shutdown;
    _BitInt(BITSIZE) chunk_start; // first odd number in the current chunk
    _BitInt(BITSIZE) *results;    // shared results buffer; slot k is chunk_start + 2*k
    size_t count;                 // number of odd values in the current chunk
    atomic_size_t cursor;         // shared work-stealing cursor into [0, count)
} pool_t;

pool_t pool = {
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .cv_start = PTHREAD_COND_INITIALIZER,
    .cv_done = PTHREAD_COND_INITIALIZER,
};

// Threads pull contiguous granules from a shared atomic cursor (work-stealing) instead
// of a static split: run length varies a lot per number, so a thread that finishes its
// granules quickly just grabs more, keeping all threads busy without false sharing
// (each granule is far larger than a cache line).
void *collatz_worker(void *arg) {
    (void)arg;
    int my_generation = 0;
    for (;;) {
        pthread_mutex_lock(&pool.mutex);
        while (pool.generation == my_generation && !pool.shutdown) {
            pthread_cond_wait(&pool.cv_start, &pool.mutex);
        }
        bool done = pool.shutdown;
        my_generation = pool.generation;
        pthread_mutex_unlock(&pool.mutex);
        if (done) {
            return NULL;
        }

        for (;;) {
            size_t begin = atomic_fetch_add_explicit(&pool.cursor, granule_size, memory_order_relaxed);
            if (begin >= pool.count) {
                break;
            }
            size_t end = begin + granule_size;
            if (end > pool.count) {
                end = pool.count;
            }
            for (size_t k = begin; k < end; k++) {
                _BitInt(BITSIZE) i = pool.chunk_start + 2wb * (_BitInt(BITSIZE))k;
                uint64_t max64;
                _BitInt(BITSIZE) v;
                if (i <= (_BitInt(BITSIZE))UINT64_MAX && collatzrun64((uint64_t)i, &max64)) {
                    v = max64;
                } else {
                    v = collatzrun(i);
                }
                pool.results[k] = v;
            }
        }

        pthread_mutex_lock(&pool.mutex);
        if (--pool.workers_left == 0) {
            pthread_cond_signal(&pool.cv_done);
        }
        pthread_mutex_unlock(&pool.mutex);
    }
}

// Hands the pool a new chunk to process and blocks until all workers are done with it.
void pool_run_chunk(_BitInt(BITSIZE) chunk_start, size_t count) {
    pthread_mutex_lock(&pool.mutex);
    pool.chunk_start = chunk_start;
    pool.count = count;
    atomic_store_explicit(&pool.cursor, 0, memory_order_relaxed);
    pool.workers_left = NUM_THREADS;
    pool.generation++;
    pthread_cond_broadcast(&pool.cv_start);
    while (pool.workers_left > 0) {
        pthread_cond_wait(&pool.cv_done, &pool.mutex);
    }
    pthread_mutex_unlock(&pool.mutex);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s start end\n", argv[0]);
        return 1;
    }
    _BitInt(BITSIZE) start, end, max, v;
    if (!atobigi(argv[1], &start)) {
        fprintf(stderr, "Invalid input: %s\n", argv[1]);
        return 1;
    }
    if (!(start & 1wb)) {
        start++; // make sure we start with an odd number
    }
    if (start < 3wb) {
        start =
            3wb; // Collatz sequence is trivial for 1 and 2 (and would loop with our assumptions).
    }
    if (!atobigi(argv[2], &end)) {
        fprintf(stderr, "Invalid input: %s\n", argv[2]);
        return 1;
    }
    // also make sure we end with an odd number
    if (!(end & 1wb)) {
        end++;
    }
    max = 0;
    _BitInt(BITSIZE) *results = malloc(chunk_size * sizeof(_BitInt(BITSIZE)));
    if (results == NULL) {
        fprintf(stderr, "Out of memory allocating results buffer.\n");
        return 1;
    }
    pool.results = results;
    pthread_t threads[NUM_THREADS];
    for (int t = 0; t < NUM_THREADS; t++) {
        pthread_create(&threads[t], NULL, collatz_worker, NULL);
    }
    // no point in checking the even numbers,
    // as they will always be smaller than the odd number before them.
    for (_BitInt(BITSIZE) chunk_start = start; chunk_start <= end; chunk_start += 2wb * chunk_size) {
        _BitInt(BITSIZE) chunk_last = chunk_start + 2wb * (chunk_size - 1);
        if (chunk_last > end) {
            chunk_last = end;
        }
        size_t count = (size_t)((chunk_last - chunk_start) / 2wb) + 1;
        pool_run_chunk(chunk_start, count);
        // results are reduced/printed strictly in order, so max and output are correct.
        for (size_t k = 0; k < count; k++) {
            _BitInt(BITSIZE) i = chunk_start + 2wb * (_BitInt(BITSIZE))k;
            v = results[k];
            if (v < 0) {
                print(i);
                printf(": overflow at ");
                print(-v);
                free(results);
                return 1;
            }
            if (v > max || i == end || i == start) {
                max = v;
                print(i);
                printf(": ");
                print(max);
                _BitInt(BITSIZE) ratio = (max + i / 2wb) / i;
                printf(" (");
                print(ratio);
                printf("x)\n");
            }
        }
    }
    pthread_mutex_lock(&pool.mutex);
    pool.shutdown = true;
    pool.generation++;
    pthread_cond_broadcast(&pool.cv_start);
    pthread_mutex_unlock(&pool.mutex);
    for (int t = 0; t < NUM_THREADS; t++) {
        pthread_join(threads[t], NULL);
    }
    free(results);
}
