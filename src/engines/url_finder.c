#include "engines/url_finder.h"

#include <string.h>
#include <strings.h>

/* Punctuation that usually ends a sentence rather than an address. */
static int trailing_punctuation(char c) {
    return c == '.' || c == ',' || c == ';' || c == ':' || c == '!' || c == '?' || c == ')' || c == ']' ||
           c == '\'' || c == '"';
}

static int stops_url(unsigned char c) {
    return c <= ' ' || c == '<' || c == '>' || c == '"';
}

int url_find_first(const char *text, char *out, size_t out_size) {
    if (out && out_size) out[0] = '\0';
    if (!text || !out || out_size == 0) return -1;
    for (const char *p = text; *p; p++) {
        size_t scheme = 0;
        if (strncasecmp(p, "https://", 8) == 0) scheme = 8;
        else if (strncasecmp(p, "http://", 7) == 0) scheme = 7;
        if (!scheme || (p > text && !stops_url((unsigned char)p[-1]) && p[-1] != '(')) continue;
        size_t len = scheme;
        while (p[len] && !stops_url((unsigned char)p[len])) len++;
        while (len > scheme && trailing_punctuation(p[len - 1])) len--;
        if (len == scheme) continue;                  /* "https://" alone */
        if (len >= out_size) return -1;
        memcpy(out, p, len);
        out[len] = '\0';
        return 0;
    }
    return -1;
}
