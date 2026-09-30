#include "utilities/sixel_encoder.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BINS        32768      /* 5 bits per channel */
#define MAX_COLOURS 256

typedef struct Buffer { char *data; size_t len, cap; int failed; } Buffer;

static void put(Buffer *b, const char *s, size_t n) {
    if (b->failed) return;
    if (b->len + n + 1 > b->cap) {
        size_t cap = b->cap ? b->cap * 2 : 65536;
        while (cap < b->len + n + 1) cap *= 2;
        char *grown = realloc(b->data, cap);
        if (!grown) { b->failed = 1; return; }
        b->data = grown;
        b->cap = cap;
    }
    memcpy(b->data + b->len, s, n);
    b->len += n;
    b->data[b->len] = '\0';
}

/* Appends a formatted number sequence (kept to literal formats). */
#define PUTF(b, ...) do { char tmp_[64]; int n_ = snprintf(tmp_, sizeof(tmp_), __VA_ARGS__); \
                          if (n_ > 0) put((b), tmp_, (size_t)n_); } while (0)

static int bin_of(const unsigned char *p) { return (p[0] >> 3) << 10 | (p[1] >> 3) << 5 | (p[2] >> 3); }

/* One run of the same sixel character, with repeat compression. */
static void put_run(Buffer *b, char ch, int count) {
    if (count >= 4) { PUTF(b, "!%d", count); put(b, &ch, 1); return; }
    while (count-- > 0) put(b, &ch, 1);
}

/* Palette by popularity: the most common 15-bit colours, each pixel then
 * mapped to its nearest palette entry. */
static int build_palette(const RgbImage *img, unsigned char palette[MAX_COLOURS][3], unsigned char *index) {
    int *count = calloc(BINS, sizeof(int));
    int *map = malloc(BINS * sizeof(int));
    if (!count || !map) { free(count); free(map); return 0; }
    size_t pixels = (size_t)img->width * img->height;
    for (size_t i = 0; i < pixels; i++) count[bin_of(&img->pixels[i * 3])]++;
    int colours = 0;
    int chosen[MAX_COLOURS];
    for (; colours < MAX_COLOURS; colours++) {
        int best = -1;
        for (int bin = 0; bin < BINS; bin++) if (count[bin] > 0 && (best < 0 || count[bin] > count[best])) best = bin;
        if (best < 0) break;
        chosen[colours] = best;
        palette[colours][0] = (unsigned char)(((best >> 10) & 31) << 3 | 4);
        palette[colours][1] = (unsigned char)(((best >> 5) & 31) << 3 | 4);
        palette[colours][2] = (unsigned char)((best & 31) << 3 | 4);
        count[best] = -count[best];                     /* taken; remembered as used */
    }
    for (int bin = 0; bin < BINS; bin++) map[bin] = -1;
    for (int c = 0; c < colours; c++) map[chosen[c]] = c;
    for (size_t i = 0; i < pixels; i++) {
        int bin = bin_of(&img->pixels[i * 3]);
        if (map[bin] < 0) {
            int r = ((bin >> 10) & 31) << 3, g = ((bin >> 5) & 31) << 3, bl = (bin & 31) << 3;
            long best_d = -1;
            for (int c = 0; c < colours; c++) {
                long dr = r - palette[c][0], dg = g - palette[c][1], db = bl - palette[c][2];
                long d = 3 * dr * dr + 4 * dg * dg + 2 * db * db;
                if (best_d < 0 || d < best_d) { best_d = d; map[bin] = c; }
            }
        }
        index[i] = (unsigned char)map[bin];
    }
    free(count);
    free(map);
    return colours;
}

#define TRANSPARENT 0xFF        /* palette index never used for a colour when masking */

char *sixel_encode(const RgbImage *img, size_t *length) { return sixel_encode_masked(img, NULL, length); }

char *sixel_encode_masked(const RgbImage *img, const unsigned char *mask, size_t *length) {
    if (!img || !img->pixels || img->width < 1 || img->height < 1) return NULL;
    size_t pixels = (size_t)img->width * img->height;
    unsigned char *index = malloc(pixels);
    unsigned char palette[MAX_COLOURS][3];
    if (!index) return NULL;
    int colours = build_palette(img, palette, index);
    if (colours == 0) { free(index); return NULL; }
    if (mask) {                                        /* keep index 255 free for "not drawn" */
        if (colours > 255) colours = 255;
        for (size_t i = 0; i < pixels; i++) {
            if (index[i] >= colours) index[i] = (unsigned char)(colours - 1);
            if (!mask[i]) index[i] = TRANSPARENT;
        }
    }

    Buffer b = { 0 };
    put(&b, "\033P0;1;0q", 8);                           /* keep what is under unset pixels */
    PUTF(&b, "\"1;1;%d;%d", img->width, img->height);
    for (int c = 0; c < colours; c++) {
        PUTF(&b, "#%d;2;%d;%d;%d", c, palette[c][0] * 100 / 255, palette[c][1] * 100 / 255, palette[c][2] * 100 / 255);
    }
    unsigned char used[MAX_COLOURS];
    for (int top = 0; top < img->height; top += 6) {
        int band = img->height - top < 6 ? img->height - top : 6;
        memset(used, 0, sizeof(used));
        for (int r = 0; r < band; r++) {
            const unsigned char *row = &index[(size_t)(top + r) * img->width];
            for (int x = 0; x < img->width; x++) if (!mask || row[x] != TRANSPARENT) used[row[x]] = 1;
        }
        int first = 1;
        for (int c = 0; c < colours; c++) {
            if (!used[c]) continue;
            if (!first) put(&b, "$", 1);                /* back to the band start for the next colour */
            first = 0;
            PUTF(&b, "#%d", c);
            char run_ch = 0;
            int run = 0;
            for (int x = 0; x < img->width; x++) {
                int bits = 0;
                for (int r = 0; r < band; r++) if (index[(size_t)(top + r) * img->width + x] == c) bits |= 1 << r;
                char ch = (char)(63 + bits);
                if (run && ch == run_ch) { run++; continue; }
                put_run(&b, run_ch, run);
                run_ch = ch;
                run = 1;
            }
            if (run_ch != 63) put_run(&b, run_ch, run);   /* trailing blanks need not be sent */
        }
        put(&b, "-", 1);
    }
    put(&b, "\033\\", 2);
    free(index);
    if (b.failed) { free(b.data); return NULL; }
    if (length) *length = b.len;
    return b.data;
}
