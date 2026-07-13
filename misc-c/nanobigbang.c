#include <stdio.h>
#include <time.h>

int main(void) {
    double prev = 0;
    const struct timespec pause_1ms = {.tv_sec = 0, .tv_nsec = 1000000};
    while (1) {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        struct tm tm;
        localtime_r(&ts.tv_sec, &tm);
        const double bb_years = 13.8e9; // approx big bang age... to unix epoch
        double fs = 1e9 * (bb_years * 365.25 * 86400.0 + ts.tv_sec) + ts.tv_nsec;
        if (fs < prev) {
            // printf("oscillating bigbang clock: %.17g\n", fs-prev);
            fs = prev;
        }
        if (fs == prev) {
            nanosleep(&pause_1ms, NULL);
            continue;
        }
        prev = fs;
        char buf[64];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
        printf("%s.%03ld : ", buf, ts.tv_nsec / 1000000);
        printf("ns bigbang clock: %.17g\n", fs);
    }
    return 0;
}
