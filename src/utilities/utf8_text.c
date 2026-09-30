#include "utilities/utf8_text.h"

#include <stdlib.h>
#include <string.h>
#include <wchar.h>

/* Decodes one character. Returns bytes consumed (>=1) and its width (>=0). */
static size_t next_char(const char *s, size_t avail, int *width) {
    mbstate_t state;
    memset(&state, 0, sizeof(state));
    wchar_t wc;
    size_t n = mbrtowc(&wc, s, avail, &state);
    if (n == (size_t)-1 || n == (size_t)-2 || n == 0) {
        *width = 1;
        return 1;
    }
    int w = wcwidth(wc);
    *width = w < 0 ? 0 : w;
    return n;
}

int utf8_columns(const char *s) {
    int total = 0;
    size_t len = strlen(s);
    for (size_t i = 0; i < len;) {
        int w;
        i += next_char(s + i, len - i, &w);
        total += w;
    }
    return total;
}

size_t utf8_fit(const char *s, size_t length, int max_columns, int *columns) {
    size_t i = 0;
    int used = 0;
    while (i < length) {
        int w;
        size_t n = next_char(s + i, length - i, &w);
        if (used + w > max_columns) break;
        used += w;
        i += n;
    }
    if (columns) *columns = used;
    return i;
}

static int push_line(TextLine **lines, int *count, int *cap, size_t off, size_t len, int cols) {
    if (*count == *cap) {
        int ncap = *cap ? *cap * 2 : 8;
        TextLine *grown = realloc(*lines, (size_t)ncap * sizeof(TextLine));
        if (!grown) return -1;
        *lines = grown;
        *cap = ncap;
    }
    (*lines)[(*count)++] = (TextLine){ off, len, cols };
    return 0;
}

int utf8_wrap(const char *text, int width, TextLine **lines) {
    *lines = NULL;
    int count = 0, cap = 0;
    if (width < 1) width = 1;
    size_t len = strlen(text);
    size_t line_start = 0, i = 0, last_space = (size_t)-1;
    int cols = 0, cols_at_space = 0;

    while (i < len) {
        if (text[i] == '\n') {
            push_line(lines, &count, &cap, line_start, i - line_start, cols);
            i++;
            line_start = i;
            cols = 0;
            last_space = (size_t)-1;
            continue;
        }
        int w;
        size_t n = next_char(text + i, len - i, &w);
        if (cols + w > width) {
            if (last_space != (size_t)-1 && last_space > line_start) {
                push_line(lines, &count, &cap, line_start, last_space - line_start, cols_at_space);
                line_start = last_space + 1;
                i = line_start;
            } else {
                push_line(lines, &count, &cap, line_start, i - line_start, cols);
                line_start = i;
            }
            cols = 0;
            last_space = (size_t)-1;
            continue;
        }
        if (text[i] == ' ') {
            last_space = i;
            cols_at_space = cols;
        }
        cols += w;
        i += n;
    }
    if (line_start < len || count == 0) {
        push_line(lines, &count, &cap, line_start, len - line_start, cols);
    }
    return count;
}

char *utf8_from_wide(const wchar_t *ws, size_t count) {
    size_t cap = count * MB_CUR_MAX + 1;
    char *out = malloc(cap);
    if (!out) return NULL;
    size_t used = 0;
    mbstate_t state;
    memset(&state, 0, sizeof(state));
    for (size_t i = 0; i < count; i++) {
        size_t n = wcrtomb(out + used, ws[i], &state);
        if (n != (size_t)-1) used += n;
    }
    out[used] = '\0';
    return out;
}
