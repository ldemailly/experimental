#include <stdio.h>
#include <string.h>
#include <time.h>

static const char *day_ordinal_suffix(int day) {
    if (day >= 11 && day <= 13) {
        return "th";
    }
    switch (day % 10) {
    case 1:
        return "st";
    case 2:
        return "nd";
    case 3:
        return "rd";
    default:
        return "th";
    }
}

size_t strftime_o(char *s, size_t max, const char *format, const struct tm *tm) {
    char expanded_fmt[1024];
    size_t out_idx = 0;
    for (size_t i = 0; format[i] != '\0'; i++) {
        if (format[i] == '%') {
            if (format[i + 1] == 'o') {
                char day_str[16];
                int len = snprintf(day_str, sizeof(day_str), "%d%s", tm->tm_mday,
                                   day_ordinal_suffix(tm->tm_mday));
                if (len < 0 || out_idx + (size_t)len >= sizeof(expanded_fmt) - 1) {
                    return 0;
                }
                memcpy(&expanded_fmt[out_idx], day_str, (size_t)len);
                out_idx += (size_t)len;
                i++;
                continue;
            } else if (format[i + 1] == '%') {
                if (out_idx + 2 >= sizeof(expanded_fmt) - 1)
                    return 0;
                expanded_fmt[out_idx++] = '%';
                expanded_fmt[out_idx++] = '%';
                i++;
                continue;
            }
        }
        if (out_idx + 1 >= sizeof(expanded_fmt) - 1) {
            return 0;
        }
        expanded_fmt[out_idx++] = format[i];
    }
    expanded_fmt[out_idx] = '\0';
    return strftime(s, max, expanded_fmt, tm);
}

int main(void) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char buf[128];
    const char *format = "%A the %o of %B, %Y";
    strftime_o(buf, sizeof(buf), format, tm_info);
    puts(buf);
    for (int d = 1; d < 5; d++) {
        tm_info->tm_mday = d;
        strftime_o(buf, sizeof(buf), format, tm_info);
        puts(buf);
    }
}
