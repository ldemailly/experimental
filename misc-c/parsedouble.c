#include <stdio.h>
#include <stdlib.h>

static inline int is_digit(char c) {
    return c >= '0' && c <= '9';
}

static inline int parse_sign(const char **s) {
    if (**s == '-') {
        (*s)++;
        return -1;
    }
    if (**s == '+') {
        (*s)++;
    }
    return 1;
}

static inline unsigned long parse_digits(const char **s) {
    unsigned long val = 0;
    while (is_digit(**s)) {
        val = val * 10 + (unsigned)(**s - '0');
        (*s)++;
    }
    return val;
}

static inline double exponent_scale(unsigned long exp, double by) {
    double scale = 1.0;
    while (exp > 0) {
        scale *= by;
        exp--;
    }
    return scale;
}

double parse_double(const char *s) {
    int sign = parse_sign(&s);
    double res = parse_digits(&s);
    if (*s == '.') {
        s++;
        const char *start = s;
        double frac = parse_digits(&s);
        unsigned long digits = (unsigned long)(s - start);
        double scale = exponent_scale(digits, 0.1);
        res += frac * scale;
    }
    if (*s == 'e' || *s == 'E') {
        s++;
        int exp_sign = parse_sign(&s);
        unsigned long exp = parse_digits(&s);
        double scale = exponent_scale(exp, 10.0);
        res = (exp_sign > 0) ? (res * scale) : (res / scale);
    }
    return res * sign;
}


int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <number>\n", argv[0]);
        return 1;
    }
    const char *input = argv[1];
    double result = parse_double(input);
    double expected = strtod(input, NULL);
    if (result != expected) {
        fprintf(stderr, "Error: parsed value %g does not match expected %g delta %g\n", result, expected, result -  expected);
    }
    printf("Parsed double: %g (%.17f %A vs %g %.17f %A)\n", result, result, result, result, expected, expected);
    return 0;
}