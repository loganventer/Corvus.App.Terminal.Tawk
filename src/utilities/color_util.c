#include "utilities/color_util.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static const int CUBE[6] = { 0, 95, 135, 175, 215, 255 };

static int nearest_cube(int v) {
    int best = 0;
    for (int i = 1; i < 6; i++) if (abs(CUBE[i] - v) < abs(CUBE[best] - v)) best = i;
    return best;
}

static long distance(int r1, int g1, int b1, int r2, int g2, int b2) {
    long dr = r1 - r2, dg = g1 - g2, db = b1 - b2;
    return dr * dr * 3 + dg * dg * 4 + db * db * 2;   /* rough perceptual weighting */
}

short color_rgb_to_xterm256(int r, int g, int b) {
    int ri = nearest_cube(r), gi = nearest_cube(g), bi = nearest_cube(b);
    long cube_d = distance(r, g, b, CUBE[ri], CUBE[gi], CUBE[bi]);
    int gray_i = (r + g + b) / 3 < 8 ? 0 : ((r + g + b) / 3 - 8) / 10;
    if (gray_i > 23) gray_i = 23;
    int gv = 8 + gray_i * 10;
    long gray_d = distance(r, g, b, gv, gv, gv);
    return gray_d < cube_d ? (short)(232 + gray_i) : (short)(16 + 36 * ri + 6 * gi + bi);
}

static int hexval(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    c = (char)tolower((unsigned char)c);
    return (c >= 'a' && c <= 'f') ? c - 'a' + 10 : -1;
}

short color_parse(const char *text) {
    if (!text) return -2;
    if (strcmp(text, "default") == 0) return -1;
    if (text[0] == '#') {
        size_t n = strlen(text + 1);
        int v[6];
        for (size_t i = 0; i < n && i < 6; i++) if ((v[i] = hexval(text[1 + i])) < 0) return -2;
        if (n == 6) return color_rgb_to_xterm256(v[0] * 16 + v[1], v[2] * 16 + v[3], v[4] * 16 + v[5]);
        if (n == 3) return color_rgb_to_xterm256(v[0] * 17, v[1] * 17, v[2] * 17);
        return -2;
    }
    char *end = NULL;
    long idx = strtol(text, &end, 10);
    return (end && *end == '\0' && idx >= 0 && idx <= 255) ? (short)idx : -2;
}

short color_xterm256_to_8(short index) {
    if (index < 0) return -1;
    if (index < 16) return (short)(index % 8);
    int r, g, b;
    if (index >= 232) {
        r = g = b = 8 + (index - 232) * 10;
    } else {
        int c = index - 16;
        r = CUBE[c / 36];
        g = CUBE[(c / 6) % 6];
        b = CUBE[c % 6];
    }
    int max = r > g ? (r > b ? r : b) : (g > b ? g : b);
    if (max < 60) return 0;                                   /* black */
    int t = max * 6 / 10;
    int bits = (r >= t ? 1 : 0) | (g >= t ? 2 : 0) | (b >= t ? 4 : 0);
    return (short)bits;                                       /* curses: 1 red, 2 green, 4 blue */
}
