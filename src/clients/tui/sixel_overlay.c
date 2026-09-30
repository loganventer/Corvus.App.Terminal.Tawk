#include "clients/tui/sixel_overlay.h"

#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void sixel_overlay_invalidate(SixelOverlay *o) { o->stale = 1; }

static int same(const ImagePlacement *a, const ImagePlacement *b) {
    return a->y == b->y && a->x == b->x && a->cols == b->cols && a->rows == b->rows &&
           a->attr == b->attr && a->source == b->source && a->page == b->page && a->round == b->round && strcmp(a->id, b->id) == 0 && strcmp(a->path, b->path) == 0;
}

static int changed(const SixelOverlay *o, const ImagePlacement *p, int count) {
    if (o->stale || o->lines != LINES || o->cols != COLS || o->shown_count != count) return 1;
    for (int i = 0; i < count; i++) if (!same(&o->shown[i], &p[i])) return 1;
    return 0;
}

static void write_all(const char *data, size_t length) {
    while (length > 0) {
        ssize_t n = write(STDOUT_FILENO, data, length);
        if (n <= 0) return;
        data += n;
        length -= (size_t)n;
    }
}

void sixel_overlay_present(SixelOverlay *o, SixelImageCache *cache, int cell_w, int cell_h,
                           const ImagePlacement *p, int count, const Message *msgs, int message_count) {
    if (count > SIXEL_OVERLAY_MAX) count = SIXEL_OVERLAY_MAX;
    if (!changed(o, p, count)) { refresh(); return; }

    /* Rows that held pixels are repainted in full, which clears them. */
    for (int i = 0; i < o->shown_count; i++) wredrawln(stdscr, o->shown[i].y, o->shown[i].rows);
    refresh();
    fflush(stdout);

    for (int i = 0; i < count; i++) {
        const SixelImage *img = NULL;
        if (p[i].message < 0) {                                /* a picture file, such as a portrait */
            img = sixel_image_cache_get_path(cache, p[i].path, p[i].cols, p[i].rows, cell_w, cell_h, p[i].round);
        } else if (p[i].message < message_count) {
            MediaPicture picture = { &msgs[p[i].message], (MediaPictureSource)p[i].source,
                                     p[i].page > 0 ? p[i].page : 1, p[i].plain, "" };
            snprintf(picture.path, sizeof(picture.path), "%s", p[i].path);
            img = sixel_image_cache_get(cache, &picture, p[i].cols, p[i].rows, cell_w, cell_h);
        }
        if (!img) continue;
        char move[32];
        int n = snprintf(move, sizeof(move), "\0337\033[%d;%dH", p[i].y + 1, p[i].x + 1);
        write_all(move, (size_t)n);
        write_all(img->data, img->length);
        write_all("\0338", 2);
    }
    memcpy(o->shown, p, sizeof(ImagePlacement) * (size_t)count);
    o->shown_count = count;
    o->lines = LINES;
    o->cols = COLS;
    o->stale = 0;
}
