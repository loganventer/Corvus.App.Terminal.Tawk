#include "utilities/rgb_image_resize.h"

#include <stdlib.h>

int rgb_image_resize(const RgbImage *src, int width, int height, RgbImage *out) {
    if (!src || !src->pixels || width < 1 || height < 1 || width > 4096 || height > 4096) return -1;
    out->pixels = malloc((size_t)width * (size_t)height * 3);
    if (!out->pixels) return -1;
    out->width = width;
    out->height = height;
    for (int y = 0; y < height; y++) {
        int y0 = (int)((long)y * src->height / height), y1 = (int)((long)(y + 1) * src->height / height);
        if (y1 <= y0) y1 = y0 + 1;
        for (int x = 0; x < width; x++) {
            int x0 = (int)((long)x * src->width / width), x1 = (int)((long)(x + 1) * src->width / width);
            if (x1 <= x0) x1 = x0 + 1;
            long r = 0, g = 0, b = 0, n = 0;
            for (int sy = y0; sy < y1 && sy < src->height; sy++) {
                const unsigned char *p = &src->pixels[((size_t)sy * src->width + x0) * 3];
                for (int sx = x0; sx < x1 && sx < src->width; sx++, p += 3) { r += p[0]; g += p[1]; b += p[2]; n++; }
            }
            unsigned char *d = &out->pixels[((size_t)y * width + x) * 3];
            d[0] = (unsigned char)(n ? r / n : 0);
            d[1] = (unsigned char)(n ? g / n : 0);
            d[2] = (unsigned char)(n ? b / n : 0);
        }
    }
    return 0;
}
