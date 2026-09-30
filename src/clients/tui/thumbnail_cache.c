#include "clients/tui/thumbnail_cache.h"
#include "clients/tui/color_pair_cache.h"
#include "utilities/color_util.h"
#include "utilities/lru_cache.h"
#include "utilities/rgb_image.h"
#include "utilities/rgb_image_file.h"

#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>

struct ThumbnailCache {
    LruCache *cells;
};

static void free_thumbnail(void *value) {
    Thumbnail *t = value;
    free(t->top);
    free(t->bottom);
    free(t);
}

ThumbnailCache *thumbnail_cache_create(int capacity) {
    ThumbnailCache *c = calloc(1, sizeof(*c));
    if (!c) return NULL;
    c->cells = lru_cache_create(capacity, free_thumbnail);
    if (!c->cells) { free(c); return NULL; }
    return c;
}

void thumbnail_cache_destroy(ThumbnailCache *c) {
    if (!c) return;
    lru_cache_destroy(c->cells);
    free(c);
}

/* Average colour of a pixel block, as an xterm-256 index. */
static short block_color(const RgbImage *img, int x0, int y0, int x1, int y1) {
    long r = 0, g = 0, b = 0, n = 0;
    if (x1 <= x0) x1 = x0 + 1;
    if (y1 <= y0) y1 = y0 + 1;
    for (int y = y0; y < y1 && y < img->height; y++) {
        for (int x = x0; x < x1 && x < img->width; x++) {
            const unsigned char *p = &img->pixels[(y * img->width + x) * 3];
            r += p[0]; g += p[1]; b += p[2]; n++;
        }
    }
    if (!n) return 0;
    return color_rgb_to_xterm256((int)(r / n), (int)(g / n), (int)(b / n));
}

static Thumbnail *build(const RgbImage *img, int max_cols, int max_rows) {
    /* Terminal cells are about twice as tall as wide; a half block splits a
     * cell into two square-ish pixels, so the picture keeps its shape. */
    int cols = max_cols;
    int rows = (int)((long)cols * img->height / img->width / 2);
    if (rows > max_rows) {
        rows = max_rows;
        cols = (int)((long)rows * 2 * img->width / img->height);
    }
    if (cols < 2 || rows < 1) return NULL;
    Thumbnail *t = calloc(1, sizeof(*t));
    if (!t) return NULL;
    t->cols = cols;
    t->rows = rows;
    t->top = malloc(sizeof(short) * (size_t)(cols * rows));
    t->bottom = malloc(sizeof(short) * (size_t)(cols * rows));
    if (!t->top || !t->bottom) { free_thumbnail(t); return NULL; }
    int px_rows = rows * 2;
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            int x0 = c * img->width / cols, x1 = (c + 1) * img->width / cols;
            int ya = (2 * r) * img->height / px_rows, yb = (2 * r + 1) * img->height / px_rows;
            int yc = (2 * r + 2) * img->height / px_rows;
            t->top[r * cols + c] = block_color(img, x0, ya, x1, yb);
            t->bottom[r * cols + c] = block_color(img, x0, yb, x1, yc);
        }
    }
    return t;
}

Thumbnail *thumbnail_from_image(const RgbImage *image, int max_cols, int max_rows) {
    if (!image || !image->pixels || max_cols < 2 || max_rows < 1 || !color_pair_cache_available()) return NULL;
    return build(image, max_cols, max_rows);
}

void thumbnail_free(Thumbnail *thumb) { if (thumb) free_thumbnail(thumb); }

static const Thumbnail *get_one(ThumbnailCache *c, const MediaPicture *picture, int max_cols, int max_rows) {
    char id[96], key[128];
    media_picture_key(picture, id, sizeof(id));
    snprintf(key, sizeof(key), "%s/%dx%d", id, max_cols, max_rows);
    Thumbnail *hit = lru_cache_get(c->cells, key);
    if (hit) return hit;
    RgbImage img;
    if (media_picture_decode(picture, &img) != 0) return NULL;
    Thumbnail *t = build(&img, max_cols, max_rows);
    rgb_image_dispose(&img);
    if (t) lru_cache_put(c->cells, key, t);
    return t;
}

const Thumbnail *thumbnail_cache_get_path(ThumbnailCache *c, const char *path, int max_cols, int max_rows) {
    if (!c || !path || !path[0] || max_cols < 1 || !color_pair_cache_available()) return NULL;
    char key[700];
    snprintf(key, sizeof(key), "path:%s/%dx%d", path, max_cols, max_rows);
    Thumbnail *hit = lru_cache_get(c->cells, key);
    if (hit) return hit;
    RgbImage img;
    if (rgb_image_load_file(path, &img) != 0) return NULL;
    Thumbnail *t = build(&img, max_cols, max_rows);
    rgb_image_dispose(&img);
    if (t) lru_cache_put(c->cells, key, t);
    return t;
}

const Thumbnail *thumbnail_cache_get(ThumbnailCache *c, MediaPicture *picture, int max_cols, int max_rows) {
    if (!c || !picture || max_cols < 2 || !color_pair_cache_available()) return NULL;
    do {
        const Thumbnail *t = get_one(c, picture, max_cols, max_rows);
        if (t) return t;
    } while (media_picture_fallback(picture));
    return NULL;
}

void thumbnail_draw(const Thumbnail *t, int y, int x) {
    if (!t) return;
    for (int r = 0; r < t->rows; r++) {
        for (int c = 0; c < t->cols; c++) {
            int pair = color_pair_cache_get(t->top[r * t->cols + c], t->bottom[r * t->cols + c]);
            if (!pair) return;
            cchar_t cell;
            setcchar(&cell, L"▀", A_NORMAL, 0, &pair);   /* ▀ : top pixel fg, bottom pixel bg */
            mvadd_wch(y + r, x + c, &cell);
        }
    }
}
