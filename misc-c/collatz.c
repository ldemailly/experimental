#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define BITSIZE 128

bool atoi(const char *str, _BitInt(BITSIZE) * out_val) {
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
    while (*str != '\0') {
        char c = *str;
        if (c < '0' || c > '9') {
            return false; // Found a non-numeric character
        }
        value = (value * 10wb) + (c - '0');
        str++;
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
    constexpr int len = (BITSIZE * 146) / 485 + 3;
    char buffer[len];
    buffer[len - 1] = '\0';
    int i = len - 2;
    int sign = (value < 0wb) ? -1 : 1;
    do {
        buffer[i--] = '0' + sign * (value % 10wb);
        value /= 10wb;
    } while (value != 0wb);
    if (sign == -1) {
        buffer[i--] = '-';
    }
    puts(buffer + i + 1);
}

bool collatz(_BitInt(BITSIZE) * n) {
    int sign = (*n < 0) ? -1 : 1;
    if ((*n & 1) == 0) {
        *n /= 2wb;
    } else {
        *n = 3wb * (*n) + 1wb;
        // check for overflow: if n was positive, 3*n + 1 should also be positive.
        int new_sign = (*n < 0) ? -1 : 1;
        if (new_sign != sign) {
            fprintf(stderr, "Overflow occurred during Collatz computation.\n");
            return false;
        }
    }
    return true;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s value\n", argv[0]);
        return 1;
    }
    _BitInt(BITSIZE) n, max;
    size_t iters = 0;
    if (!atoi(argv[1], &n)) {
        fprintf(stderr, "Invalid input: %s\n", argv[1]);
        return 1;
    }
    max = n;
    do {
        // print(n);
        if (n > max) {
            max = n;
        }
        iters++;
    } while (collatz(&n) && n != 1wb);
    printf("Took %zu iterations, max value was ", iters);
    print(max);
}