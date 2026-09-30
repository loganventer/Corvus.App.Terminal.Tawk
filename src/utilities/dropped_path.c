#include "utilities/dropped_path.h"
#include "utilities/path_util.h"
#include "utilities/str_util.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int hex(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    c = (char)tolower((unsigned char)c);
    return (c >= 'a' && c <= 'f') ? c - 'a' + 10 : -1;
}

static void percent_decode(char *s) {
    char *w = s;
    for (char *r = s; *r; r++) {
        if (r[0] == '%' && hex(r[1]) >= 0 && hex(r[2]) >= 0) {
            *w++ = (char)(hex(r[1]) * 16 + hex(r[2]));
            r += 2;
        } else {
            *w++ = *r;
        }
    }
    *w = '\0';
}

static void unescape_backslashes(char *s) {
    char *w = s;
    for (char *r = s; *r; r++) {
        if (r[0] == '\\' && r[1] && strchr(" ()[]'\"&;$!", r[1])) r++;
        *w++ = *r;
    }
    *w = '\0';
}

int dropped_path_resolve(const char *pasted, char *out, size_t size) {
    if (!pasted) return -1;
    char buf[4096];
    str_copy(buf, sizeof(buf), pasted);
    char *s = str_trim(buf);
    if (strchr(s, '\n')) return -1;                     /* multi-line pastes are text, not files */
    size_t len = strlen(s);
    if (len >= 2 && ((s[0] == '\'' && s[len - 1] == '\'') || (s[0] == '"' && s[len - 1] == '"'))) {
        s[len - 1] = '\0';
        s++;
    } else {
        unescape_backslashes(s);
    }
    if (strncmp(s, "file://", 7) == 0) {
        s += 7;
        if (strncmp(s, "localhost/", 10) == 0) s += 9;
        percent_decode(s);
    }
    if (isalpha((unsigned char)s[0]) && s[1] == ':' && (s[2] == '\\' || s[2] == '/')) {
        char mapped[4096];
        snprintf(mapped, sizeof(mapped), "/mnt/%c/%s", tolower((unsigned char)s[0]), s + 3);
        for (char *p = mapped; *p; p++) if (*p == '\\') *p = '/';
        str_copy(out, size, mapped);
    } else if (s[0] == '~') {
        path_expand_home(s, out, size);
    } else {
        str_copy(out, size, s);
    }
    return (out[0] == '/' && path_is_regular_file(out)) ? 0 : -1;
}
