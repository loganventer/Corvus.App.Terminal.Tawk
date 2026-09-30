#include "clients/tui/sixel_image_cache.h"
#include "utilities/lru_cache.h"
#include "utilities/rgb_image_file.h"
#include "utilities/rgb_image_resize.h"
#include "utilities/sixel_encoder.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct SixelImageCache {
    LruCache *images;
};

static void free_image(void *value) {
    SixelImage *s = value;
    free(s->data);
    free(s);
}

SixelImageCache *sixel_image_cache_create(int capacity) {
    SixelImageCache *c = calloc(1, sizeof(*c));
    if (!c) return NULL;
    c->images = lru_cache_create(capacity, free_image);
    if (!c->images) { free(c); return NULL; }
    return c;
}

void sixel_image_cache_destroy(SixelImageCache *c) {
    if (!c) return;
    lru_cache_destroy(c->images);
    free(c);
}

static SixelImage *encode_fitted(const RgbImage *src, int cols, int rows, int cell_w, int cell_h);

/* Keeps the centred square of the picture (profile pictures are square already). */
static void crop_square(RgbImage *img) {
    int side = img->width < img->height ? img->width : img->height;
    if (img->width == img->height || side < 1) return;
    int ox = (img->width - side) / 2, oy = (img->height - side) / 2;
    unsigned char *px = malloc((size_t)side * side * 3);
    if (!px) return;
    for (int y = 0; y < side; y++) memcpy(px + (size_t)y * side * 3, img->pixels + (((size_t)(y + oy) * img->width) + ox) * 3, (size_t)side * 3);
    free(img->pixels);
    img->pixels = px;
    img->width = img->height = side;
}

/* A round portrait: scaled to the box's largest square, pixels outside the
 * circle left undrawn so the background shows through. */
static SixelImage *encode_round(const RgbImage *src, int cols, int rows, int cell_w, int cell_h) {
    int side = cols * cell_w < rows * cell_h ? cols * cell_w : rows * cell_h;
    if (side < 2) return NULL;
    RgbImage scaled;
    if (rgb_image_resize(src, side, side, &scaled) != 0) return NULL;
    unsigned char *mask = malloc((size_t)side * side);
    SixelImage *out = NULL;
    if (mask) {
        double r = side / 2.0;
        for (int y = 0; y < side; y++) {
            for (int x = 0; x < side; x++) {
                double dx = x + 0.5 - r, dy = y + 0.5 - r;
                mask[(size_t)y * side + x] = dx * dx + dy * dy <= r * r;
            }
        }
        size_t len = 0;
        char *data = sixel_encode_masked(&scaled, mask, &len);
        if (data && (out = malloc(sizeof(*out)))) { out->data = data; out->length = len; }
        else free(data);
        free(mask);
    }
    rgb_image_dispose(&scaled);
    return out;
}

const SixelImage *sixel_image_cache_get(SixelImageCache *c, const MediaPicture *picture,
                                        int cols, int rows, int cell_w, int cell_h) {
    if (!c || !picture || cols < 1 || rows < 1) return NULL;
    char id[96], key[160];
    media_picture_key(picture, id, sizeof(id));
    snprintf(key, sizeof(key), "%s/%dx%d/%dx%d", id, cols, rows, cell_w, cell_h);
    SixelImage *hit = lru_cache_get(c->images, key);
    if (hit) return hit;

    RgbImage img = { 0 };
    if (media_picture_decode(picture, &img) != 0) return NULL;
    SixelImage *out = encode_fitted(&img, cols, rows, cell_w, cell_h);
    rgb_image_dispose(&img);
    if (out) lru_cache_put(c->images, key, out);
    return out;
}

const SixelImage *sixel_image_cache_get_path(SixelImageCache *c, const char *path, int cols, int rows, int cell_w, int cell_h,
                                             int round) {
    if (!c || !path || !path[0] || cols < 1 || rows < 1) return NULL;
    char key[700];
    snprintf(key, sizeof(key), "path:%s/%dx%d/%dx%d%s", path, cols, rows, cell_w, cell_h, round ? "/round" : "");
    SixelImage *hit = lru_cache_get(c->images, key);
    if (hit) return hit;
    RgbImage img = { 0 };
    if (rgb_image_load_file(path, &img) != 0) return NULL;
    if (round) crop_square(&img);                           /* a circle needs a square to start from */
    SixelImage *out = round ? encode_round(&img, cols, rows, cell_w, cell_h) : encode_fitted(&img, cols, rows, cell_w, cell_h);
    rgb_image_dispose(&img);
    if (out) lru_cache_put(c->images, key, out);
    return out;
}

/* Fits a picture inside the cell box (keeping its shape) and encodes it. */
static SixelImage *encode_fitted(const RgbImage *src, int cols, int rows, int cell_w, int cell_h) {
    const RgbImage *img = src;
    /* Fit inside the cell box, keeping the picture's shape. */
    int box_w = cols * cell_w, box_h = rows * cell_h;
    int w = box_w, h = (int)((long)box_w * img->height / img->width);
    if (h > box_h) { h = box_h; w = (int)((long)box_h * img->width / img->height); }
    if (w < 1) w = 1;
    if (h < 1) h = 1;
    RgbImage scaled;
    SixelImage *out = NULL;
    if (rgb_image_resize(img, w, h, &scaled) == 0) {
        size_t len = 0;
        char *data = sixel_encode(&scaled, &len);
        rgb_image_dispose(&scaled);
        if (data && (out = malloc(sizeof(*out)))) { out->data = data; out->length = len; }
        else free(data);
    }
    return out;
}
