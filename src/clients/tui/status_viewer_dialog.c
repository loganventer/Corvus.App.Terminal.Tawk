#include "clients/tui/status_viewer_dialog.h"
#include "clients/tui/color_pair_cache.h"
#include "clients/tui/media_picture.h"
#include "clients/tui/tui_draw.h"
#include "clients/tui/tui_palette.h"
#include "utilities/clock_util.h"
#include "utilities/color_util.h"
#include "utilities/str_util.h"
#include "utilities/utf8_text.h"

#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WIDTH  76
#define HEIGHT 30
#define DOT    "\xC2\xB7"

void status_viewer_dialog_open(StatusViewerDialog *d, const char *jid, const char *title, int start, int count) {
    memset(d, 0, sizeof(*d));
    str_copy(d->author_jid, sizeof(d->author_jid), jid ? jid : "");
    str_copy(d->title, sizeof(d->title), title ? title : "");
    d->count = count;
    d->index = start >= 0 && start < count ? start : 0;
    d->moved = 1;
    text_field_init(&d->reply, 700);
    text_field_allow_newlines(&d->reply, 1);
    d->open = 1;
}

/* WhatsApp's quick reactions to a status. */
static const char *const QUICK[8] = {
    "\xF0\x9F\x98\x8D", "\xF0\x9F\x98\x82", "\xF0\x9F\x98\xAE", "\xF0\x9F\x98\xA2",   /* 😍 😂 😮 😢 */
    "\xF0\x9F\x91\x8F", "\xF0\x9F\x8E\x89", "\xF0\x9F\x92\xAF", "\xF0\x9F\x99\x8F",   /* 👏 🎉 💯 🙏 */
};

const char *status_viewer_dialog_quick_emoji(int index) { return index >= 0 && index < 8 ? QUICK[index] : QUICK[0]; }
char *status_viewer_dialog_reply_text(const StatusViewerDialog *d) { return text_field_text(&d->reply); }
void  status_viewer_dialog_paste(StatusViewerDialog *d, const char *utf8) { if (d->replying) text_field_paste(&d->reply, utf8); }

static void ask(StatusViewerDialog *d, StatusViewerIntent intent, int emoji) {
    d->intent = intent;
    d->intent_emoji = emoji;
}

static PopupResult step(StatusViewerDialog *d, int dir) {
    int next = d->index + dir;
    if (next < 0) return POPUP_NONE;
    if (next >= d->count) { d->open = 0; d->finished = 1; return POPUP_CLOSED; }   /* past the last: the owner decides who is next */
    d->index = next;
    d->moved = 1;
    d->replying = 0;
    d->elapsed_ms = 0;                                  /* the next status gets its full time */
    return POPUP_CHANGED;
}

PopupResult status_viewer_dialog_tick(StatusViewerDialog *d, int64_t now_ms, int64_t show_ms, int hold) {
    if (!d->open) return POPUP_NONE;
    int64_t since = d->ticked_ms ? now_ms - d->ticked_ms : 0;
    d->ticked_ms = now_ms;
    d->show_ms = show_ms;
    if (hold || d->replying || since <= 0) return POPUP_NONE;
    if (since > 1000) since = 1000;                     /* a stalled loop does not skip statuses */
    d->elapsed_ms += since;
    return d->elapsed_ms >= show_ms ? step(d, 1) : POPUP_NONE;
}

/* Typing a reply: Enter sends it, Esc drops it, everything else edits it. */
static PopupResult reply_key(StatusViewerDialog *d, int is_key, int ch) {
    if (!is_key && ch == 27) { d->replying = 0; return POPUP_CHANGED; }
    if ((!is_key && (ch == '\n' || ch == '\r')) || (is_key && ch == KEY_ENTER)) {
        if (text_field_length(&d->reply) > 0) ask(d, STATUS_VIEWER_INTENT_REPLY, 0);
        return POPUP_CHANGED;
    }
    text_field_key(&d->reply, is_key, ch);
    return POPUP_CHANGED;
}

PopupResult status_viewer_dialog_key(StatusViewerDialog *d, int is_key, int ch) {
    if (d->replying) return reply_key(d, is_key, ch);
    if (!is_key && (ch == 27 || ch == 'q')) { d->open = 0; return POPUP_CLOSED; }
    if (!d->mine && !is_key) {                         /* someone else's status: answer it */
        if (ch == 'r') { d->replying = 1; text_field_set(&d->reply, ""); return POPUP_CHANGED; }
        if (ch == 'l') { ask(d, STATUS_VIEWER_INTENT_LIKE, 0); return POPUP_NONE; }
        if (ch >= '1' && ch <= '8') { ask(d, STATUS_VIEWER_INTENT_REACT, ch - '1'); return POPUP_NONE; }
    }
    if ((is_key && ch == KEY_RIGHT) || (!is_key && (ch == ' ' || ch == 'n'))) return step(d, 1);
    if ((is_key && ch == KEY_LEFT) || (!is_key && ch == 'p')) return step(d, -1);
    if ((!is_key && (ch == '\n' || ch == '\r')) || (is_key && ch == KEY_ENTER)) return POPUP_CHOSEN;
    if (d->mine && ((!is_key && ch == 'v') || (is_key && ch == KEY_UP))) ask(d, STATUS_VIEWER_INTENT_VIEWERS, 0);
    return POPUP_NONE;
}

PopupResult status_viewer_dialog_click(StatusViewerDialog *d, int y, int x) {
    if (!ui_rect_contains(d->last_rect, y, x)) { d->open = 0; return POPUP_CLOSED; }
    if (d->viewers_button.w > 0 && ui_rect_contains(d->viewers_button, y, x)) { ask(d, STATUS_VIEWER_INTENT_VIEWERS, 0); return POPUP_NONE; }
    if (!d->mine) {
        for (int i = 0; i < 8; i++) {
            if (d->reactions[i].w > 0 && ui_rect_contains(d->reactions[i], y, x)) { ask(d, STATUS_VIEWER_INTENT_REACT, i); return POPUP_NONE; }
        }
        if (d->like_button.w > 0 && ui_rect_contains(d->like_button, y, x)) { ask(d, STATUS_VIEWER_INTENT_LIKE, 0); return POPUP_NONE; }
        if (d->reply_button.w > 0 && ui_rect_contains(d->reply_button, y, x)) {
            d->replying = 1;
            text_field_set(&d->reply, "");
            return POPUP_CHANGED;
        }
    }
    if (d->prev_arrow.w > 0 && ui_rect_contains(d->prev_arrow, y, x)) return step(d, -1);
    if (d->next_arrow.w > 0 && ui_rect_contains(d->next_arrow, y, x)) return step(d, 1);
    if (ui_rect_contains(d->media_rect, y, x)) return POPUP_CHOSEN;
    if (ui_rect_contains(d->prev_zone, y, x)) return step(d, -1);
    if (ui_rect_contains(d->next_zone, y, x)) return step(d, 1);
    return POPUP_NONE;
}

/* A bar per status: the ones before full, the shown one filling as its time
 * runs, the rest light. */
static void draw_progress(const StatusViewerDialog *d, int y, UiRect box) {
    int n = d->count > 0 ? d->count : 1;
    int span = box.w - 4, gap = n > 1 ? 1 : 0;
    int each = (span - gap * (n - 1)) / n;
    if (each < 1) each = 1;
    int x = box.x + 2;
    for (int i = 0; i < n && x < box.x + box.w - 2; i++) {
        int filled = i < d->index ? each : i > d->index ? 0
                   : d->show_ms > 0 ? (int)(d->elapsed_ms * each / d->show_ms) + 1 : each;
        if (filled > each) filled = each;
        for (int k = 0; k < each && x + k < box.x + box.w - 2; k++) {
            int on = k < filled;
            tui_text(y, x + k, 1, on ? "\xE2\x94\x81" : "\xE2\x94\x80",
                     on ? tui_palette_attr(THEME_SLOT_ACCENT) | ATTR_BOLD : tui_palette_attr(THEME_SLOT_DIM));
        }
        x += each + gap;
    }
}

/* Draws `text` wrapped and centred in `r`, in colour pair `pair` (0 for the theme). */
static void draw_centred_text(UiRect r, const char *text, int pair, int theme_attr) {
    TextLine *lines = NULL;
    int n = utf8_wrap(text, r.w - 4 > 8 ? r.w - 4 : r.w, &lines);
    int y = r.y + (r.h - n) / 2;
    if (y < r.y) y = r.y;
    for (int i = 0; i < n && y + i < r.y + r.h; i++) {
        char line[512];
        size_t len = lines[i].length < sizeof(line) - 1 ? lines[i].length : sizeof(line) - 1;
        memcpy(line, text + lines[i].offset, len);
        line[len] = '\0';
        int cols = utf8_columns(line);
        int x = r.x + (r.w - cols) / 2;
        if (pair) {
            attr_set(A_BOLD, (short)pair, NULL);
            mvaddnstr(y + i, x < r.x ? r.x : x, line, -1);
            attr_set(A_NORMAL, 0, NULL);
        } else {
            tui_text(y + i, x < r.x ? r.x : x, r.w, line, theme_attr | ATTR_BOLD);
        }
    }
    free(lines);
}

/* A text status: its words, white on its own colour when the terminal can show it. */
static void draw_text_status(UiRect r, const StatusUpdate *u) {
    int pair = 0;
    if (u->background_argb && color_pair_cache_available()) {
        short bg = color_rgb_to_xterm256((int)((u->background_argb >> 16) & 0xFF), (int)((u->background_argb >> 8) & 0xFF),
                                         (int)(u->background_argb & 0xFF));
        pair = color_pair_cache_get(15, bg);
    }
    if (pair) {
        attr_set(A_NORMAL, (short)pair, NULL);
        for (int y = r.y; y < r.y + r.h; y++) mvhline(y, r.x, ' ', r.w);
        attr_set(A_NORMAL, 0, NULL);
    } else {
        tui_fill(r, tui_palette_attr(THEME_SLOT_BUBBLE_THEM));
    }
    draw_centred_text(r, u->text && *u->text ? u->text : "", pair, tui_palette_attr(THEME_SLOT_BUBBLE_THEM));
}

/* A photo or video: the best picture there is so far, centred, with a
 * note while the full file is still on its way. */
static void draw_media_status(StatusViewerDialog *d, UiRect r, const StatusUpdate *u, ThumbnailCache *thumbs,
                              const MediaSources *sources) {
    Message m;
    message_init(&m);
    str_copy(m.id, sizeof(m.id), u->id);
    m.type = u->type;
    str_copy(m.media_path, sizeof(m.media_path), u->media_path);
    if (u->thumbnail && u->thumbnail_len > 0) message_set_thumbnail(&m, u->thumbnail, u->thumbnail_len);
    MediaPicture pic;
    const Thumbnail *t = NULL;
    if (media_picture_for(&m, sources, 1, &pic)) t = thumbnail_cache_get(thumbs, &pic, r.w, r.h);
    if (t) {
        int y = r.y + (r.h - t->rows) / 2, x = r.x + (r.w - t->cols) / 2;
        thumbnail_draw(t, y, x);
        d->media_rect = (UiRect){ y, x, t->rows, t->cols };
    } else {
        tui_text_center(r.y + r.h / 2, r.x, r.w, u->type == MESSAGE_TYPE_VIDEO ? "\xF0\x9F\x8E\xAC Video" : "\xF0\x9F\x93\xB7 Photo",
                        tui_palette_attr(THEME_SLOT_DIM));
        d->media_rect = r;
    }
    if (!u->media_path[0]) tui_text_center(r.y + r.h - 1, r.x, r.w, " Downloading\xE2\x80\xA6 ", tui_palette_attr(THEME_SLOT_DIM));
    message_dispose(&m);
}

/* The row under someone else's status: 😍 😂 … ❤️ like · r reply, or the reply being typed. */
static void draw_answers(StatusViewerDialog *d, int y, UiRect box) {
    int base = tui_palette_attr(THEME_SLOT_BASE), accent = tui_palette_attr(THEME_SLOT_ACCENT) | ATTR_BOLD;
    if (d->replying) {
        int used = tui_text(y, box.x + 2, 8, "Reply: ", accent);
        UiRect field = { y, box.x + 2 + used, 1, box.w - 4 - used };
        text_field_render(&d->reply, field, tui_palette_attr(THEME_SLOT_COMPOSER), 1, &d->caret);
        return;
    }
    int x = box.x + 2;
    for (int i = 0; i < 8; i++) {
        int used = tui_text(y, x, 3, QUICK[i], base);
        d->reactions[i] = (UiRect){ y, x, 1, used };
        x += used + 1;
    }
    x += 1;
    int used = tui_text(y, x, 12, "\xE2\x9D\xA4\xEF\xB8\x8F like", accent);   /* ❤️ */
    d->like_button = (UiRect){ y, x, 1, used };
    x += used + 3;
    used = tui_text(y, x, 12, "\xE2\x86\xA9 reply", accent);                     /* ↩ */
    d->reply_button = (UiRect){ y, x, 1, used };
}

void status_viewer_dialog_render(StatusViewerDialog *d, UiRect a, const StatusUpdate *items, int count,
                                 ThumbnailCache *thumbs, const MediaSources *sources, int use_24h, const char *viewers_label) {
    d->count = count;
    if (d->index >= count) d->index = count > 0 ? count - 1 : 0;
    int w = a.w < WIDTH ? a.w : WIDTH;
    int h = a.h < HEIGHT ? a.h : HEIGHT;
    UiRect box = { a.y + (a.h - h) / 2, a.x + (a.w - w) / 2, h, w };
    d->last_rect = box;
    int base = tui_palette_attr(THEME_SLOT_BASE);
    tui_fill(box, base);
    tui_box(box, NULL, tui_palette_attr(THEME_SLOT_BORDER));
    d->media_rect = (UiRect){ 0, 0, 0, 0 };
    if (count <= 0) {
        tui_text_center(box.y + box.h / 2, box.x, box.w, "These statuses have expired.", base | ATTR_DIM);
        return;
    }
    const StatusUpdate *u = &items[d->index];

    draw_progress(d, box.y + 1, box);
    char when[32], head[256];
    clock_format_relative(u->timestamp, use_24h, when, sizeof(when));
    snprintf(head, sizeof(head), "%s " DOT " %s", d->title, when);
    tui_text(box.y + 2, box.x + 2, box.w - 12, head, base | ATTR_BOLD);
    char pos[24];
    snprintf(pos, sizeof(pos), "%d/%d", d->index + 1, count);
    tui_text_right(box.y + 2, box.x + box.w - 2, 10, pos, base | ATTR_DIM);

    int caption = (u->type == MESSAGE_TYPE_IMAGE || u->type == MESSAGE_TYPE_VIDEO) && u->text && *u->text;
    UiRect body = { box.y + 4, box.x + 2, box.h - 7 - (caption ? 2 : 0), box.w - 4 };
    if (u->type == MESSAGE_TYPE_IMAGE || u->type == MESSAGE_TYPE_VIDEO) {
        draw_media_status(d, body, u, thumbs, sources);
        if (caption) draw_centred_text((UiRect){ body.y + body.h, body.x, 2, body.w }, u->text, 0, base);
    } else {
        draw_text_status(body, u);
    }
    d->prev_zone = (UiRect){ body.y, box.x, body.h, body.w / 4 };
    d->next_zone = (UiRect){ body.y, box.x + box.w - body.w / 4, body.h, body.w / 4 };

    /* Arrows to click beside the status, in the margin so they never cover it;
     * three rows tall to be easy to hit. */
    int mid = body.y + body.h / 2, arrow = tui_palette_attr(THEME_SLOT_ACCENT) | ATTR_BOLD;
    d->prev_arrow = d->next_arrow = (UiRect){ 0, 0, 0, 0 };
    if (body.h >= 3 && box.w >= 12) {
        if (d->index > 0) {
            tui_text(mid, box.x + 1, 1, "\xE2\x97\x80", arrow);                    /* ◀ */
            d->prev_arrow = (UiRect){ mid - 1, box.x, 3, 2 };
        }
        tui_text(mid, box.x + box.w - 2, 1, "\xE2\x96\xB6", arrow);                /* ▶ */
        d->next_arrow = (UiRect){ mid - 1, box.x + box.w - 2, 3, 2 };
    }

    /* Under your own statuses: who saw it, a button to the list. Under
     * anyone else's: the quick emoji, a like, and a reply. */
    d->viewers_button = d->like_button = d->reply_button = (UiRect){ 0, 0, 0, 0 };
    memset(d->reactions, 0, sizeof(d->reactions));
    d->mine = u->from_me;
    d->caret.visible = 0;
    if (!d->mine) draw_answers(d, box.y + box.h - 2, box);
    if (viewers_label && viewers_label[0]) {
        char label[96];
        snprintf(label, sizeof(label), " %s ", viewers_label);
        int cols = utf8_columns(label), y = box.y + box.h - 2, x = box.x + (box.w - cols) / 2;
        tui_text(y, x, cols, label, tui_palette_attr(THEME_SLOT_ACCENT) | ATTR_BOLD);
        d->viewers_button = (UiRect){ y, x, 1, cols };
    }

    const char *hint = u->type == MESSAGE_TYPE_VIDEO ? " \xE2\x86\x90 \xE2\x86\x92 step " DOT " Enter play " DOT " Esc back "
                     : u->type == MESSAGE_TYPE_IMAGE ? " \xE2\x86\x90 \xE2\x86\x92 step " DOT " Enter full size " DOT " Esc back "
                                                     : " \xE2\x86\x90 \xE2\x86\x92 step " DOT " Esc back ";
    if (d->viewers_button.w > 0) hint = " \xE2\x86\x90 \xE2\x86\x92 step " DOT " \xE2\x86\x91 or v viewers " DOT " Esc back ";
    else if (d->replying) hint = " Enter send " DOT " Shift+Enter new line " DOT " Esc cancel ";
    else if (!d->mine) hint = " \xE2\x86\x90 \xE2\x86\x92 step " DOT " 1-8 react " DOT " l like " DOT " r reply " DOT " Esc back ";
    tui_text_center(box.y + box.h - 1, box.x, box.w, hint, tui_palette_attr(THEME_SLOT_BORDER));
}
