#include "clients/tui/image_viewer.h"
#include "clients/tui/tui_draw.h"
#include "clients/tui/tui_palette.h"
#include "utilities/clock_util.h"
#include "utilities/path_util.h"
#include "clients/tui/media_picture.h"
#include "utilities/str_util.h"

#include <ncurses.h>
#include <stdio.h>
#include <string.h>

#define DOT "\xC2\xB7"

static int has_file(const Message *m) { return m->media_path[0] && path_is_regular_file(m->media_path); }

int image_viewer_can_show(const Message *m) {
    if (!media_picture_applies(m)) return 0;
    if (m->type == MESSAGE_TYPE_IMAGE) return has_file(m) || m->thumbnail_len > 0;
    return 1;                                      /* videos and PDFs: a placeholder at worst */
}

void image_viewer_open(ImageViewer *v, const Message *m, const MediaSources *sources) {
    memset(v, 0, sizeof(*v));
    str_copy(v->message_id, sizeof(v->message_id), m->id);
    v->page = 1;
    v->sources = sources;
    v->open = 1;
}

/* Pages of the PDF shown, or 0 when it is not a downloaded PDF. */
static int page_count(const ImageViewer *v, const Message *m) {
    if (!m || !media_picture_is_pdf(m) || !has_file(m) || !v->sources || !v->sources->pages) return 0;
    return v->sources->pages->page_count(v->sources->pages, m->media_path);
}

static ImageViewerAction turn(ImageViewer *v, const Message *msgs, int count, int dir);

void image_viewer_close(ImageViewer *v) { v->open = 0; }

void image_viewer_open_portrait(ImageViewer *v, const char *jid, const char *name, const char *path) {
    memset(v, 0, sizeof(*v));
    v->portrait = 1;
    str_copy(v->portrait_jid, sizeof(v->portrait_jid), jid);
    str_copy(v->portrait_name, sizeof(v->portrait_name), name);
    str_copy(v->portrait_path, sizeof(v->portrait_path), path ? path : "");
    v->open = 1;
}

/* A profile picture: the name, the picture fitted to the window, Esc to close. */
static int render_portrait(ImageViewer *v, UiRect area, ThumbnailCache *thumbs, int pixel_images, ImagePlacement *placement) {
    int bg = tui_palette_attr(THEME_SLOT_BASE), dim = tui_palette_attr(THEME_SLOT_DIM);
    int header = tui_palette_attr(THEME_SLOT_HEADER);
    tui_fill(area, bg);
    tui_fill((UiRect){ area.y, area.x, 1, area.w }, header);
    char top[200];
    snprintf(top, sizeof(top), " %s " DOT " profile picture", v->portrait_name);
    tui_text(area.y, area.x, area.w, top, header | ATTR_BOLD);
    tui_text_right(area.y + area.h - 1, area.x + area.w, area.w, "Esc close ", dim);
    UiRect box = { area.y + 2, area.x + 2, area.h - 4, area.w - 4 };
    if (!v->portrait_path[0]) {
        tui_text_center(box.y + box.h / 2, box.x, box.w, "Loading the picture\xE2\x80\xA6", dim);
        return 0;
    }
    /* Square pictures: as wide as twice the height in cells keeps them square. */
    int rows = box.h, cols = rows * 2;
    if (cols > box.w) { cols = box.w; rows = cols / 2; }
    UiRect pic = { box.y + (box.h - rows) / 2, box.x + (box.w - cols) / 2, rows, cols };
    if (pixel_images && placement) {
        memset(placement, 0, sizeof(*placement));
        placement->message = -1;
        placement->y = pic.y;
        placement->x = pic.x;
        placement->cols = pic.w;
        placement->rows = pic.h;
        placement->attr = bg;
        str_copy(placement->path, sizeof(placement->path), v->portrait_path);
        str_copy(placement->id, sizeof(placement->id), v->portrait_jid);
        return 1;
    }
    const Thumbnail *t = thumbnail_cache_get_path(thumbs, v->portrait_path, pic.w, pic.h);
    if (t) thumbnail_draw(t, pic.y + (pic.h - t->rows) / 2, pic.x + (pic.w - t->cols) / 2);
    return 0;
}

const Message *image_viewer_current(const ImageViewer *v, const Message *msgs, int count, int *index) {
    for (int i = 0; i < count; i++) {
        if (strcmp(msgs[i].id, v->message_id) == 0) { if (index) *index = i; return &msgs[i]; }
    }
    return NULL;
}

/* Moves to the previous (-1) or next (+1) photo or video in the chat. */
static ImageViewerAction step(ImageViewer *v, const Message *msgs, int count, int dir) {
    int at = -1;
    if (!image_viewer_current(v, msgs, count, &at)) return IMAGE_VIEWER_NONE;
    for (int i = at + dir; i >= 0 && i < count; i += dir) {
        if (!image_viewer_can_show(&msgs[i])) continue;
        str_copy(v->message_id, sizeof(v->message_id), msgs[i].id);
        v->page = 1;
        return IMAGE_VIEWER_CHANGED;
    }
    return IMAGE_VIEWER_NONE;
}

ImageViewerAction image_viewer_key(ImageViewer *v, const Message *msgs, int count, int is_key, int ch) {
    if (v->portrait) {
        if ((!is_key && (ch == 27 || ch == 'q' || ch == '\n' || ch == '\r')) || (is_key && ch == KEY_ENTER)) {
            v->open = 0;
            return IMAGE_VIEWER_CLOSED;
        }
        return IMAGE_VIEWER_NONE;
    }
    if (!is_key && (ch == 27 || ch == 'q')) { v->open = 0; return IMAGE_VIEWER_CLOSED; }
    if (!is_key && (ch == 'o' || ch == '\n' || ch == '\r')) return IMAGE_VIEWER_OPEN_OUTSIDE;
    if (is_key && ch == KEY_ENTER) return IMAGE_VIEWER_OPEN_OUTSIDE;
    /* ← → and PgUp PgDn turn pages of a PDF (then move on); ↑ ↓ always move between items. */
    if (is_key && (ch == KEY_UP)) return step(v, msgs, count, -1);
    if (is_key && (ch == KEY_DOWN)) return step(v, msgs, count, 1);
    if ((is_key && (ch == KEY_LEFT || ch == KEY_PPAGE)) || (!is_key && (ch == 'h' || ch == 'k' || ch == 127 || ch == 8))) {
        return turn(v, msgs, count, -1);
    }
    if ((is_key && (ch == KEY_RIGHT || ch == KEY_NPAGE)) || (!is_key && (ch == 'l' || ch == 'j' || ch == ' '))) {
        return turn(v, msgs, count, 1);
    }
    if (is_key && ch == KEY_HOME && v->page != 1) { v->page = 1; return IMAGE_VIEWER_CHANGED; }
    if (is_key && ch == KEY_END) {
        const Message *m = image_viewer_current(v, msgs, count, NULL);
        int pages = page_count(v, m);
        if (pages > 1 && v->page != pages) { v->page = pages; return IMAGE_VIEWER_CHANGED; }
    }
    return IMAGE_VIEWER_NONE;
}

/* The next or previous page of a PDF, or the next or previous item past its ends. */
static ImageViewerAction turn(ImageViewer *v, const Message *msgs, int count, int dir) {
    const Message *m = image_viewer_current(v, msgs, count, NULL);
    int pages = page_count(v, m);
    if (pages > 1 && v->page + dir >= 1 && v->page + dir <= pages) {
        v->page += dir;
        return IMAGE_VIEWER_CHANGED;
    }
    return step(v, msgs, count, dir);
}

ImageViewerAction image_viewer_wheel(ImageViewer *v, const Message *msgs, int count, int delta) {
    if (v->portrait) return IMAGE_VIEWER_NONE;
    return turn(v, msgs, count, delta < 0 ? -1 : 1);
}

/* Left third: previous, right third: next, middle: close. */
ImageViewerAction image_viewer_click(ImageViewer *v, const Message *msgs, int count, int y, int x) {
    (void)y;
    if (v->portrait) { v->open = 0; return IMAGE_VIEWER_CLOSED; }
    UiRect r = v->last_rect;
    if (r.w <= 0) return IMAGE_VIEWER_NONE;
    if (x < r.x + r.w / 3) return turn(v, msgs, count, -1);
    if (x >= r.x + r.w * 2 / 3) return turn(v, msgs, count, 1);
    v->open = 0;
    return IMAGE_VIEWER_CLOSED;
}

/* Position among the chat's photos and videos, for "3 of 12". */
static void position(const Message *msgs, int count, int at, int *nth, int *total) {
    *nth = *total = 0;
    for (int i = 0; i < count; i++) {
        if (!image_viewer_can_show(&msgs[i])) continue;
        (*total)++;
        if (i <= at) *nth = *total;
    }
}

int image_viewer_render(ImageViewer *v, UiRect area, const Message *msgs, int count, ThumbnailCache *thumbs,
                        const NameResolver *names, int use_24h, int pixel_images, ImagePlacement *placement) {
    v->last_rect = area;
    if (v->portrait) return render_portrait(v, area, thumbs, pixel_images, placement);
    int bg = tui_palette_attr(THEME_SLOT_BASE);
    int dim = tui_palette_attr(THEME_SLOT_DIM);
    tui_fill(area, bg);
    int at = -1;
    const Message *m = image_viewer_current(v, msgs, count, &at);
    if (!m || area.h < 6 || area.w < 20) {
        tui_text_center(area.y + area.h / 2, area.x, area.w, "This photo is no longer loaded", dim);
        return 0;
    }

    /* Top line: who and when, position; bottom line: caption and keys. */
    char who[96] = "", day[48], when[16], top[256];
    if (names && names->resolve) names->resolve(names->ctx, m->from_me ? "" : m->sender_jid, who, sizeof(who));
    if (m->from_me) str_copy(who, sizeof(who), "You");
    clock_format_day(m->timestamp, day, sizeof(day));
    clock_format_time(m->timestamp, use_24h, when, sizeof(when));
    int nth, total;
    position(msgs, count, at, &nth, &total);
    snprintf(top, sizeof(top), " %s " DOT " %s %s", who[0] ? who : m->sender_name, day, when);
    int header = tui_palette_attr(THEME_SLOT_HEADER);
    tui_fill((UiRect){ area.y, area.x, 1, area.w }, header);
    tui_text(area.y, area.x, area.w - 12, top, header | ATTR_BOLD);
    int pages = page_count(v, m);
    if (v->page > pages && pages > 0) v->page = pages;
    char pos[64];
    if (pages > 1) snprintf(pos, sizeof(pos), "page %d of %d " DOT " %d of %d ", v->page, pages, nth, total);
    else snprintf(pos, sizeof(pos), "%d of %d ", nth, total);
    tui_text_right(area.y, area.x + area.w, 30, pos, header);

    const char *keys = m->type == MESSAGE_TYPE_VIDEO
        ? "\xE2\x96\xB6 Enter plays " DOT " \xE2\x86\x90 \xE2\x86\x92 browse " DOT " Esc close "
        : pages > 1
        ? "\xE2\x86\x90 \xE2\x86\x92 pages " DOT " \xE2\x86\x91 \xE2\x86\x93 other media " DOT " o open outside tawk " DOT " Esc close "
        : "\xE2\x86\x90 \xE2\x86\x92 browse " DOT " o open outside tawk " DOT " Esc close ";
    int keys_w = tui_text_right(area.y + area.h - 1, area.x + area.w, area.w / 2, keys, dim);
    if (m->text && m->text[0]) {
        char caption[300];
        snprintf(caption, sizeof(caption), " %s", m->text);
        for (char *c = caption; *c; c++) if (*c == '\n') *c = ' ';
        tui_text(area.y + area.h - 1, area.x, area.w - keys_w - 1, caption, bg);
    }

    /* The picture, fitted to the space between the two lines and centred. */
    UiRect box = { area.y + 2, area.x + 2, area.h - 4, area.w - 4 };
    MediaPicture picture;
    const Thumbnail *t = NULL;
    int have = media_picture_for(m, v->sources, v->page, &picture);
    picture.plain = 1;                                  /* the page itself, without the inline badge */
    if (thumbs && have) t = thumbnail_cache_get(thumbs, &picture, box.w, box.h);
    if (!t) {
        tui_text_center(box.y + box.h / 2, box.x, box.w, "This picture cannot be shown here; press o to open it", dim);
        return 0;
    }
    int y = box.y + (box.h - t->rows) / 2, x = box.x + (box.w - t->cols) / 2;
    if (media_picture_is_pdf(m) && picture.source != MEDIA_PICTURE_PAGE) {
        tui_text_center(area.y + area.h - 2, area.x, area.w,
                        has_file(m) ? "rendering the page\xE2\x80\xA6 (needs pdftoppm from poppler)" : "downloading the document\xE2\x80\xA6", dim);
    } else if (m->type == MESSAGE_TYPE_IMAGE && picture.source != MEDIA_PICTURE_FILE) {
        tui_text_center(area.y + area.h - 2, area.x, area.w, "downloading the full photo\xE2\x80\xA6", dim);
    }
    if (pixel_images && placement) {
        memset(placement, 0, sizeof(*placement));
        placement->message = at;
        placement->y = y;
        placement->x = x;
        placement->cols = t->cols;
        placement->rows = t->rows;
        placement->attr = bg;
        placement->source = (int)picture.source;
        placement->page = picture.page;
        placement->plain = 1;
        str_copy(placement->path, sizeof(placement->path), picture.path);
        str_copy(placement->id, sizeof(placement->id), m->id);
        return 1;
    }
    thumbnail_draw(t, y, x);
    return 0;
}
