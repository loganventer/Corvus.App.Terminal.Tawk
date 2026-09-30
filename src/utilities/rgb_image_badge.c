#include "utilities/rgb_image_badge.h"

#include <math.h>
#include <stdlib.h>

static void blend(unsigned char *p, int r, int g, int b, double alpha) {
    p[0] = (unsigned char)(p[0] * (1 - alpha) + r * alpha);
    p[1] = (unsigned char)(p[1] * (1 - alpha) + g * alpha);
    p[2] = (unsigned char)(p[2] * (1 - alpha) + b * alpha);
}

void rgb_image_draw_play_badge(RgbImage *img) {
    if (!img || !img->pixels || img->width < 8 || img->height < 8) return;
    int w = img->width, h = img->height;
    double cx = w / 2.0, cy = h / 2.0;
    double radius = (w < h ? w : h) * 0.16;
    if (radius < 6) radius = 6;
    /* Triangle pointing right, centred slightly right of the disc centre. */
    double tri = radius * 0.55;
    double ax = cx - tri * 0.45, bx = cx + tri * 0.85;
    for (int y = (int)(cy - radius - 1); y <= (int)(cy + radius + 1); y++) {
        if (y < 0 || y >= h) continue;
        for (int x = (int)(cx - radius - 1); x <= (int)(cx + radius + 1); x++) {
            if (x < 0 || x >= w) continue;
            double dx = x + 0.5 - cx, dy = y + 0.5 - cy;
            double d = sqrt(dx * dx + dy * dy);
            if (d > radius + 0.5) continue;
            double edge = d > radius - 0.5 ? radius + 0.5 - d : 1.0;    /* soft rim */
            unsigned char *p = &img->pixels[((size_t)y * w + x) * 3];
            blend(p, 0, 0, 0, 0.55 * edge);
            if (d > radius - radius * 0.08) blend(p, 255, 255, 255, 0.85 * edge);
            double px = x + 0.5, py = y + 0.5;
            double half = tri * (bx - px) / (bx - ax);                  /* triangle half-height at px */
            if (px >= ax && px <= bx && fabs(py - cy) <= half) blend(p, 255, 255, 255, 0.95);
        }
    }
}

int rgb_image_video_placeholder(RgbImage *out) {
    int w = 640, h = 360;
    out->pixels = malloc((size_t)w * h * 3);
    if (!out->pixels) return -1;
    out->width = w;
    out->height = h;
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            unsigned char *p = &out->pixels[((size_t)y * w + x) * 3];
            int shade = 28 + (int)(22.0 * y / h) + (int)(10.0 * x / w);
            p[0] = (unsigned char)shade;
            p[1] = (unsigned char)(shade + 4);
            p[2] = (unsigned char)(shade + 12);
        }
    }
    return 0;
}

static void fill_rect(RgbImage *img, int x0, int y0, int x1, int y1, int r, int g, int b) {
    for (int y = y0 < 0 ? 0 : y0; y < y1 && y < img->height; y++) {
        for (int x = x0 < 0 ? 0 : x0; x < x1 && x < img->width; x++) {
            unsigned char *p = &img->pixels[((size_t)y * img->width + x) * 3];
            p[0] = (unsigned char)r;
            p[1] = (unsigned char)g;
            p[2] = (unsigned char)b;
        }
    }
}

void rgb_image_draw_document_badge(RgbImage *img) {
    if (!img || !img->pixels || img->width < 16 || img->height < 16) return;
    int side = (img->width < img->height ? img->width : img->height) / 5;
    if (side < 10) side = 10;
    int x0 = side / 4, y1 = img->height - side / 4, y0 = y1 - side * 3 / 5, x1 = x0 + side;
    fill_rect(img, x0, y0, x1, y1, 214, 48, 49);                         /* red tab */
    int bar = (y1 - y0) / 6 > 1 ? (y1 - y0) / 6 : 1;                     /* three white lines */
    for (int i = 0; i < 3; i++) {
        int y = y0 + bar + i * bar * 5 / 3;
        fill_rect(img, x0 + side / 6, y, x1 - side / 6 - (i == 2 ? side / 3 : 0), y + bar, 255, 255, 255);
    }
}

int rgb_image_page_placeholder(RgbImage *out) {
    int w = 420, h = 594;                                                 /* A4 proportions */
    out->pixels = malloc((size_t)w * h * 3);
    if (!out->pixels) return -1;
    out->width = w;
    out->height = h;
    fill_rect(out, 0, 0, w, h, 250, 250, 247);
    for (int y = 70; y < h - 60; y += 26) {
        int len = (y / 26) % 4 == 3 ? w / 2 : w - 120;
        fill_rect(out, 60, y, 60 + len, y + 8, 205, 205, 200);
    }
    return 0;
}
