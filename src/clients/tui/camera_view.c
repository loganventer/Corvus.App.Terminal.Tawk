#include "clients/tui/camera_view.h"
#include "clients/tui/thumbnail_cache.h"
#include "clients/tui/tui_draw.h"
#include "clients/tui/tui_palette.h"
#include "utilities/rgb_image_resize.h"
#include "utilities/sixel_encoder.h"
#include "utilities/str_util.h"

#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void drop_frame(CameraView *v) {
    free(v->frame.pixels);
    v->frame = (RgbImage){ 0, 0, NULL };
}

void camera_view_open(CameraView *v) {
    drop_frame(v);
    memset(v, 0, sizeof(*v));
    v->open = 1;
    v->phase = CAMERA_VIEW_LIVE;
}

void camera_view_close(CameraView *v) {
    drop_frame(v);
    memset(v, 0, sizeof(*v));
}

void camera_view_set_frame(CameraView *v, const RgbImage *f) {
    if (v->phase == CAMERA_VIEW_REVIEW || !f || !f->pixels) return;
    size_t bytes = (size_t)f->width * (size_t)f->height * 3;
    if (v->frame.width != f->width || v->frame.height != f->height || !v->frame.pixels) {
        drop_frame(v);
        v->frame.pixels = malloc(bytes);
        if (!v->frame.pixels) return;
        v->frame.width = f->width;
        v->frame.height = f->height;
    }
    memcpy(v->frame.pixels, f->pixels, bytes);
}

void camera_view_review(CameraView *v, const char *photo) {
    v->phase = CAMERA_VIEW_REVIEW;
    v->video = 0;
    str_copy(v->photo, sizeof(v->photo), photo);
}

void camera_view_recording(CameraView *v) {
    v->phase = CAMERA_VIEW_RECORDING;
    v->seconds = 0;
}

void camera_view_review_video(CameraView *v, const char *path, int seconds) {
    v->phase = CAMERA_VIEW_REVIEW;
    v->video = 1;
    v->seconds = seconds;
    str_copy(v->photo, sizeof(v->photo), path);
}

void camera_view_live(CameraView *v) {
    v->phase = CAMERA_VIEW_LIVE;
    v->photo[0] = '\0';
    v->video = v->seconds = 0;
}

CameraViewAction camera_view_key(const CameraView *v, int is_key, int ch) {
    int enter = (!is_key && (ch == '\n' || ch == '\r' || ch == ' ')) || (is_key && ch == KEY_ENTER);
    int video_key = !is_key && (ch == 'v' || ch == 'V');
    if (!is_key && (ch == 27 || ch == 'q')) return CAMERA_VIEW_CANCEL;
    if (v->phase == CAMERA_VIEW_LIVE) return enter ? CAMERA_VIEW_SNAP : video_key ? CAMERA_VIEW_START_VIDEO : CAMERA_VIEW_NONE;
    if (v->phase == CAMERA_VIEW_RECORDING) return enter || video_key ? CAMERA_VIEW_STOP_VIDEO : CAMERA_VIEW_NONE;
    if (enter) return CAMERA_VIEW_USE;
    if (!is_key && (ch == 'r' || ch == 'R')) return CAMERA_VIEW_RETAKE;
    if (v->video && !is_key && (ch == 'p' || ch == 'P')) return CAMERA_VIEW_PLAY;
    return CAMERA_VIEW_NONE;
}

void camera_view_render(CameraView *v, UiRect a, int pixel_images) {
    int base = tui_palette_attr(THEME_SLOT_BASE);
    char title[64];
    const char *hint;
    int title_attr = tui_palette_attr(THEME_SLOT_BORDER);
    if (v->phase == CAMERA_VIEW_RECORDING) {
        snprintf(title, sizeof(title), "\xE2\x97\x8F REC %d:%02d", v->seconds / 60, v->seconds % 60);
        title_attr = tui_palette_attr(THEME_SLOT_WARN) | ATTR_BOLD;
        hint = "V or Space stop \xC2\xB7 Esc discard";
    } else if (v->phase == CAMERA_VIEW_REVIEW && v->video) {
        snprintf(title, sizeof(title), "\xF0\x9F\x8E\xAC Your video %d:%02d", v->seconds / 60, v->seconds % 60);
        hint = "Enter use video \xC2\xB7 P play \xC2\xB7 R retake \xC2\xB7 Esc cancel";
    } else if (v->phase == CAMERA_VIEW_REVIEW) {
        snprintf(title, sizeof(title), "\xF0\x9F\x93\xB7 Your photo");
        hint = "Enter use photo \xC2\xB7 R retake \xC2\xB7 Esc cancel";
    } else {
        snprintf(title, sizeof(title), "\xF0\x9F\x93\xB7 Camera");
        hint = "Space take photo \xC2\xB7 V record video \xC2\xB7 Esc cancel";
    }
    tui_box(a, title, title_attr);
    UiRect inside = { a.y + 1, a.x + 1, a.h - 3, a.w - 2 };
    tui_fill((UiRect){ a.y + 1, a.x + 1, a.h - 2, a.w - 2 }, base);
    tui_text_center(a.y + a.h - 2, a.x, a.w, hint, tui_palette_attr(THEME_SLOT_DIM));
    v->picture = (UiRect){ 0, 0, 0, 0 };
    if (inside.h < 2 || inside.w < 4) return;
    if (!v->frame.pixels) {
        tui_text_center(inside.y + inside.h / 2, inside.x, inside.w, "Starting the camera\xE2\x80\xA6",
                        tui_palette_attr(THEME_SLOT_DIM));
        return;
    }
    if (pixel_images) {                               /* Sixel goes over these cells after the update */
        v->picture = inside;
        return;
    }
    Thumbnail *t = thumbnail_from_image(&v->frame, inside.w, inside.h);
    if (!t) return;
    thumbnail_draw(t, inside.y + (inside.h - t->rows) / 2, inside.x + (inside.w - t->cols) / 2);
    thumbnail_free(t);
}

static void write_all(const char *data, size_t length) {
    while (length > 0) {
        ssize_t n = write(STDOUT_FILENO, data, length);
        if (n <= 0) return;
        data += n;
        length -= (size_t)n;
    }
}

void camera_view_present_pixels(const CameraView *v, int cell_w, int cell_h) {
    UiRect r = v->picture;
    if (!v->open || !v->frame.pixels || r.w < 1 || r.h < 1 || cell_w < 1 || cell_h < 1) return;
    /* Fit the frame into the cells, keeping its shape, and centre it. */
    int box_w = r.w * cell_w, box_h = r.h * cell_h;
    int w = box_w, h = (int)((long)box_w * v->frame.height / v->frame.width);
    if (h > box_h) { h = box_h; w = (int)((long)box_h * v->frame.width / v->frame.height); }
    h -= h % 6;                                        /* whole Sixel bands, so nothing spills below */
    if (w < 1 || h < 6) return;
    RgbImage fitted;
    if (rgb_image_resize(&v->frame, w, h, &fitted) != 0) return;
    size_t length = 0;
    char *sixel = sixel_encode(&fitted, &length);
    rgb_image_dispose(&fitted);
    if (!sixel) return;
    int col = r.x + (r.w - (w + cell_w - 1) / cell_w) / 2;
    int row = r.y + (r.h - (h + cell_h - 1) / cell_h) / 2;
    char move[32];
    int n = snprintf(move, sizeof(move), "\0337\033[%d;%dH", row + 1, col + 1);
    fflush(stdout);
    write_all(move, (size_t)n);
    write_all(sixel, length);
    write_all("\0338", 2);
    free(sixel);
}
