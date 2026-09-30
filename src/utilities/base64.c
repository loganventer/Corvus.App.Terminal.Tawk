#include "utilities/base64.h"

#include <stdlib.h>
#include <string.h>

static int value(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+' || c == '-') return 62;
    if (c == '/' || c == '_') return 63;
    return -1;
}

unsigned char *base64_decode(const char *text, size_t max_out, int *out_len) {
    *out_len = 0;
    if (!text) return NULL;
    size_t len = strlen(text);
    size_t cap = len / 4 * 3 + 3;
    if (cap > max_out) return NULL;
    unsigned char *out = malloc(cap ? cap : 1);
    if (!out) return NULL;
    unsigned int acc = 0;
    int bits = 0;
    size_t n = 0;
    for (size_t i = 0; i < len; i++) {
        char c = text[i];
        if (c == '=' ) break;
        if (c == '\n' || c == '\r' || c == ' ') continue;
        int v = value(c);
        if (v < 0) { free(out); return NULL; }
        acc = (acc << 6) | (unsigned int)v;
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            out[n++] = (unsigned char)((acc >> bits) & 0xFF);
        }
    }
    *out_len = (int)n;
    return out;
}

char *base64_encode(const unsigned char *data, size_t len) {
    static const char TABLE[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    char *out = malloc((len + 2) / 3 * 4 + 1);
    if (!out) return NULL;
    size_t o = 0;
    for (size_t i = 0; i < len; i += 3) {
        unsigned int v = (unsigned int)data[i] << 16;
        if (i + 1 < len) v |= (unsigned int)data[i + 1] << 8;
        if (i + 2 < len) v |= data[i + 2];
        out[o++] = TABLE[(v >> 18) & 63];
        out[o++] = TABLE[(v >> 12) & 63];
        out[o++] = i + 1 < len ? TABLE[(v >> 6) & 63] : '=';
        out[o++] = i + 2 < len ? TABLE[v & 63] : '=';
    }
    out[o] = '\0';
    return out;
}
