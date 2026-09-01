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

bool collatz(_BitInt(BITSIZE) * n) {
    if ((*n & 1) == 0) {
        *n /= 2wb;
    } else {
        if (*n > maxcollatz) {
            fprintf(stderr, "Overflow would occur during Collatz computation.\n");
            return false;
        }
        *n = 3wb * (*n) + 1wb;
    }
    return true;
}

// Returns the max reached. -negative if overflow would occur.
_BitInt(BITSIZE) collatzrun(_BitInt(BITSIZE) n) {
    _BitInt(BITSIZE) max = n;
    // as we run everything in sequence, as soon as we dip below previous
    // value, we're done.
    _BitInt(BITSIZE) prev = n - 1;
    bool ok;
    do {
        if (n > max) {
            max = n;
        }
    } while ((ok = collatz(&n)) && n > prev);
    if (!ok) {
        return -max; // return negative max to indicate overflow
    }
    return max;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s start end\n", argv[0]);
        return 1;
    }
    _BitInt(BITSIZE) start, end, max, v;
    if (!atoi(argv[1], &start)) {
        fprintf(stderr, "Invalid input: %s\n", argv[1]);
        return 1;
    }
    if (!(start & 1wb)) {
        start++; // make sure we start with an odd number
    }
    if (start < 3wb) {
        start = 3wb; // Collatz sequence is trivial for 1 and 2 (and would loop with our assumptions).
    }
    if (!atoi(argv[2], &end)) {
        fprintf(stderr, "Invalid input: %s\n", argv[2]);
        return 1;
    }
    // also make sure we end with an odd number
    if (!(end & 1wb)) {
        end++;
    }
    max = 0;
    // no point in checking the even numbers,
    // as they will always be smaller than the odd number before them.
    for (_BitInt(BITSIZE) i = start; i <= end; i += 2) {
        v = collatzrun(i);
        if (v < 0) {
            print(i);
            printf(": overflow at ");
            print(-v);
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
