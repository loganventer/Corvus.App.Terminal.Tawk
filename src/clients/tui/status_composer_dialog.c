#include "clients/tui/status_composer_dialog.h"
#include "clients/tui/status_colour.h"
#include "clients/tui/tui_draw.h"
#include "clients/tui/tui_palette.h"
#include "core/icon_glyphs.h"
#include "utilities/str_util.h"
#include "utilities/utf8_text.h"

#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#define WIDTH 70
#define CTRL(c) ((c) & 0x1F)

void status_composer_dialog_open(StatusComposerDialog *d, int max_chars) {
    memset(d, 0, sizeof(*d));
    d->kind = STATUS_KIND_TEXT;
    text_field_init(&d->input, max_chars);
    text_field_allow_newlines(&d->input, 1);
    d->open = 1;
}

static void switch_kind(StatusComposerDialog *d, int step) {
    d->kind = (StatusKind)(((int)d->kind + step + STATUS_KIND_COUNT) % STATUS_KIND_COUNT);
    d->error[0] = '\0';
}

static StatusComposerRequest close_it(StatusComposerDialog *d) {
    d->open = 0;
    d->caret.visible = 0;
    return STATUS_REQUEST_CLOSED;
}

StatusComposerRequest status_composer_dialog_key(StatusComposerDialog *d, int is_key, int ch) {
    if (!is_key && ch == 27) return close_it(d);
    if (!is_key && ch == '\t') { switch_kind(d, 1); return STATUS_REQUEST_REDRAW; }
    if (is_key && ch == KEY_BTAB) { switch_kind(d, -1); return STATUS_REQUEST_REDRAW; }
    if ((!is_key && (ch == '\n' || ch == '\r')) || (is_key && ch == KEY_ENTER)) return d->busy ? STATUS_REQUEST_NONE : STATUS_REQUEST_POST;
    if (!is_key && ch == CTRL('o') && status_kind_has_media(d->kind)) return STATUS_REQUEST_CHOOSE_FILE;
    if (!is_key && ch == CTRL('b') && !status_kind_has_media(d->kind)) { d->background++; return STATUS_REQUEST_REDRAW; }
    if (text_field_key(&d->input, is_key, ch)) { d->error[0] = '\0'; return STATUS_REQUEST_REDRAW; }
    return STATUS_REQUEST_NONE;
}

StatusComposerRequest status_composer_dialog_click(StatusComposerDialog *d, int y, int x) {
    if (!ui_rect_contains(d->last_rect, y, x) || ui_rect_contains(d->cancel_button, y, x)) return close_it(d);
    for (int i = 0; i < STATUS_KIND_COUNT; i++) {
        if (ui_rect_contains(d->tabs[i], y, x)) { d->kind = (StatusKind)i; d->error[0] = '\0'; return STATUS_REQUEST_REDRAW; }
    }
    if (ui_rect_contains(d->post_button, y, x)) return d->busy ? STATUS_REQUEST_NONE : STATUS_REQUEST_POST;
    if (status_kind_has_media(d->kind)) {
        if (ui_rect_contains(d->choose_button, y, x)) return STATUS_REQUEST_CHOOSE_FILE;
        if (ui_rect_contains(d->camera_button, y, x)) return STATUS_REQUEST_CAMERA;
    } else if (ui_rect_contains(d->background_button, y, x)) {
        d->background++;
        return STATUS_REQUEST_REDRAW;
    }
    return STATUS_REQUEST_NONE;
}

void status_composer_dialog_paste(StatusComposerDialog *d, const char *utf8) {
    text_field_paste(&d->input, utf8);
    d->error[0] = '\0';
}

void status_composer_dialog_set_file(StatusComposerDialog *d, const char *path, StatusKind kind) {
    str_copy(d->path, sizeof(d->path), path ? path : "");
    if (status_kind_has_media(kind)) d->kind = kind;
    d->error[0] = '\0';
}

void  status_composer_dialog_error(StatusComposerDialog *d, const char *why) { str_copy(d->error, sizeof(d->error), why ? why : ""); }
char *status_composer_dialog_text(const StatusComposerDialog *d) { return text_field_text(&d->input); }

static const char *hint_for(StatusKind kind) {
    switch (kind) {
        case STATUS_KIND_TEXT: return "Type a status";
        case STATUS_KIND_LINK: return "Paste a link, with words around it if you like";
        default:               return "Add a caption (optional)";
    }
}

static int input_rows(StatusKind kind) { return status_kind_has_media(kind) ? 2 : 5; }

static void draw_tabs(StatusComposerDialog *d, int y, UiRect box, int base) {
    int x = box.x + 2;
    for (int i = 0; i < STATUS_KIND_COUNT; i++) {
        char label[24];
        snprintf(label, sizeof(label), " %s ", status_kind_label((StatusKind)i));
        int attr = i == (int)d->kind ? tui_palette_attr(THEME_SLOT_SIDEBAR_SELECTED) | ATTR_BOLD : base | ATTR_DIM;
        int used = tui_text(y, x, box.x + box.w - 2 - x, label, attr);
        d->tabs[i] = (UiRect){ y, x, 1, used };
        x += used + 1;
    }
}

/* The chosen file with its size, and the buttons to choose one. */
static void draw_media_line(StatusComposerDialog *d, int y, UiRect box, int base, int has_camera) {
    const char *choose = "[ Choose file ]";
    const char *camera = d->kind == STATUS_KIND_VIDEO ? "[ Record video ]" : "[ Take photo ]";
    int cw = utf8_columns(choose), kw = utf8_columns(camera);
    int right = box.x + box.w - 2;
    int accent = tui_palette_attr(THEME_SLOT_ACCENT) | ATTR_BOLD;
    int camera_ok = has_camera;                         /* both tabs: a photo, or a video (V in the viewfinder) */
    d->camera_button = camera_ok ? (UiRect){ y, right - kw, 1, kw } : (UiRect){ 0, 0, 0, 0 };
    if (camera_ok) { tui_text(y, right - kw, kw, camera, accent); right -= kw + 1; }
    tui_text(y, right - cw, cw, choose, accent);
    d->choose_button = (UiRect){ y, right - cw, 1, cw };
    right -= cw + 1;

    char line[1200];
    struct stat st;
    if (d->path[0] && stat(d->path, &st) == 0) {
        const char *slash = strrchr(d->path, '/');
        snprintf(line, sizeof(line), ICON_FILE " %s (%.1f MB)", slash ? slash + 1 : d->path, (double)st.st_size / (1024.0 * 1024.0));
        tui_text(y, box.x + 2, right - box.x - 2, line, base);
    } else {
        tui_text(y, box.x + 2, right - box.x - 2, "No file chosen (Ctrl+O, or paste one with Alt+V)", base | ATTR_DIM);
    }
}

void status_composer_dialog_render(StatusComposerDialog *d, UiRect a, const char *background_name, uint32_t background_argb,
                                   int has_camera) {
    int rows = input_rows(d->kind);
    int w = a.w < WIDTH ? a.w : WIDTH;
    int h = rows + 12;
    if (h > a.h) h = a.h;
    UiRect box = { a.y + (a.h - h) / 2, a.x + (a.w - w) / 2, h, w };
    d->last_rect = box;
    int base = tui_palette_attr(THEME_SLOT_BASE);
    tui_fill(box, base);
    tui_box(box, "New status", tui_palette_attr(THEME_SLOT_BORDER));

    int y = box.y + 1;
    draw_tabs(d, y, box, base);
    y += 2;

    UiRect frame = { y, box.x + 2, rows + 2, box.w - 4 };
    tui_box(frame, NULL, tui_palette_attr(THEME_SLOT_ACCENT));
    UiRect inside = { frame.y + 1, frame.x + 1, rows, frame.w - 2 };
    int preview = status_kind_has_media(d->kind) ? 0 : status_colour_attr(background_argb);   /* as it will look */
    int field = preview ? preview : tui_palette_attr(THEME_SLOT_COMPOSER);
    text_field_render(&d->input, inside, field, !d->busy, &d->caret);
    if (text_field_length(&d->input) == 0) tui_text(inside.y, inside.x, inside.w, hint_for(d->kind), field | ATTR_DIM);
    y = frame.y + frame.h;
    char count[32];
    snprintf(count, sizeof(count), "%d/%d", text_field_length(&d->input), d->input.max_chars);
    tui_text_right(y, frame.x + frame.w, 12, count, base | ATTR_DIM);
    y++;

    d->choose_button = d->camera_button = d->background_button = (UiRect){ 0, 0, 0, 0 };
    if (status_kind_has_media(d->kind)) {
        draw_media_line(d, y, box, base, has_camera);
    } else {
        char line[96];
        snprintf(line, sizeof(line), "Background: \xE2\x97\x80 %s \xE2\x96\xB6", background_name ? background_name : "");   /* ◀ ▶ */
        int used = tui_text(y, box.x + 2, box.w - 4, line, tui_palette_attr(THEME_SLOT_ACCENT) | ATTR_BOLD);
        if (preview) used += 1 + tui_text(y, box.x + 3 + used, 4, "    ", preview);    /* a swatch of the colour */
        d->background_button = (UiRect){ y, box.x + 2, 1, used };
        tui_text(y, box.x + 3 + used, box.w - 5 - used, " (Ctrl+B)", base | ATTR_DIM);
    }
    y += 2;
    if (d->error[0]) tui_text(y, box.x + 2, box.w - 4, d->error, tui_palette_attr(THEME_SLOT_WARN) | ATTR_BOLD);

    const char *cancel = "  Cancel  ", *post = d->busy ? "  Posting\xE2\x80\xA6  " : "  Post  ";
    int cw = utf8_columns(cancel), pw = utf8_columns(post);
    int bx = box.x + (box.w - cw - pw - 4) / 2, by = box.y + box.h - 2;
    tui_text(by, bx, cw, cancel, base);
    tui_text(by, bx + cw + 4, pw, post, tui_palette_attr(THEME_SLOT_SIDEBAR_SELECTED) | ATTR_BOLD | (d->busy ? ATTR_DIM : 0));
    d->cancel_button = (UiRect){ by, bx, 1, cw };
    d->post_button = (UiRect){ by, bx + cw + 4, 1, pw };
    tui_text_center(box.y + box.h - 1, box.x, box.w, " Tab type \xC2\xB7 Shift+Enter new line \xC2\xB7 Enter post \xC2\xB7 Esc close ", tui_palette_attr(THEME_SLOT_BORDER));
}
