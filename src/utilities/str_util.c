#include "utilities/str_util.h"

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

size_t str_copy(char *dst, size_t dst_size, const char *src) {
    if (!src) src = "";
    size_t len = strlen(src);
    if (dst && dst_size > 0) {
        size_t n = len < dst_size - 1 ? len : dst_size - 1;
        memcpy(dst, src, n);
        dst[n] = '\0';
    }
    return len;
}

char *str_dup(const char *src) {
    return src ? strdup(src) : NULL;
}

char *str_trim(char *s) {
    if (!s) return s;
    while (*s && isspace((unsigned char)*s)) s++;
    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1])) s[--len] = '\0';
    return s;
}

int str_starts_with(const char *s, const char *prefix) {
    return s && prefix && strncmp(s, prefix, strlen(prefix)) == 0;
}

int str_parse_bool(const char *s, int fallback) {
    if (!s) return fallback;
    if (!strcasecmp(s, "true") || !strcasecmp(s, "yes") || !strcasecmp(s, "on") || !strcmp(s, "1")) return 1;
    if (!strcasecmp(s, "false") || !strcasecmp(s, "no") || !strcasecmp(s, "off") || !strcmp(s, "0")) return 0;
    return fallback;
}

int str_parse_int(const char *s, int min, int max, int fallback) {
    if (!s || !*s) return fallback;
    errno = 0;
    char *end = NULL;
    long v = strtol(s, &end, 10);
    if (errno != 0 || !end || *end != '\0') return fallback;
    if (v < min) v = min;
    if (v > max) v = max;
    return (int)v;
}

void str_strip_controls(char *s) {
    if (!s) return;
    for (unsigned char *p = (unsigned char *)s; *p; p++) {
        if (*p < 0x20 || *p == 0x7f) *p = ' ';
    }
}
