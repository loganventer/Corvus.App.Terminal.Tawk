#include "clients/tui/status_viewers_dialog.h"
#include "clients/tui/tui_draw.h"
#include "clients/tui/tui_palette.h"
#include "utilities/clock_util.h"
#include "utilities/str_util.h"

#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define WIDTH 56
#define DOT   "\xC2\xB7"

void status_viewers_dialog_open(StatusViewersDialog *d) {
    memset(d, 0, sizeof(*d));
    d->open = 1;
}

static void clamp(StatusViewersDialog *d) {
    if (d->selected >= d->count) d->selected = d->count - 1;
    if (d->selected < 0) d->selected = 0;
}

PopupResult status_viewers_dialog_key(StatusViewersDialog *d, int is_key, int ch) {
    if ((!is_key && (ch == 27 || ch == 'q')) || (is_key && ch == KEY_LEFT)) { d->open = 0; return POPUP_CLOSED; }
    if (is_key && ch == KEY_UP) d->selected--;
    else if (is_key && ch == KEY_DOWN) d->selected++;
    else if (is_key && ch == KEY_HOME) d->selected = 0;
    else if (is_key && ch == KEY_END) d->selected = d->count - 1;
    else return POPUP_NONE;
    clamp(d);
    return POPUP_CHANGED;
}

PopupResult status_viewers_dialog_click(StatusViewersDialog *d, int y, int x) {
    if (!ui_rect_contains(d->last_rect, y, x)) { d->open = 0; return POPUP_CLOSED; }
    return POPUP_NONE;
}

void status_viewers_dialog_wheel(StatusViewersDialog *d, int delta) {
    d->selected += delta;
    clamp(d);
}

void status_viewers_dialog_render(StatusViewersDialog *d, UiRect a, const StatusViewer *viewers, int count,
                                  void (*name_of)(void *ctx, const char *jid, char *out, size_t size), void *ctx, int use_24h) {
    d->count = count < STATUS_VIEWERS_ROWS ? count : STATUS_VIEWERS_ROWS;
    clamp(d);
    int likes = 0;
    for (int i = 0; i < d->count; i++) likes += viewers[i].reaction[0] != '\0';
    int w = a.w < WIDTH ? a.w : WIDTH;
    int h = d->count + 3;                           /* grows with the list, up to the screen */
    if (h < 8) h = 8;
    if (h > a.h - 4) h = a.h - 4;
    if (h < 6) h = a.h;
    UiRect box = { a.y + (a.h - h) / 2, a.x + (a.w - w) / 2, h, w };
    d->last_rect = box;
    int base = tui_palette_attr(THEME_SLOT_BASE);
    char title[64];
    snprintf(title, sizeof(title), likes ? "Viewed by %d " DOT " \xE2\x9D\xA4\xEF\xB8\x8F %d" : "Viewed by %d", d->count, likes);
    tui_fill(box, base);
    tui_box(box, title, tui_palette_attr(THEME_SLOT_BORDER));

    int rows = box.h - 3;
    if (d->selected < d->scroll) d->scroll = d->selected;
    if (d->selected >= d->scroll + rows) d->scroll = d->selected - rows + 1;
    if (d->count == 0) {
        tui_text_center(box.y + box.h / 2 - 1, box.x, box.w, "No views yet.", base | ATTR_DIM);
        tui_text_center(box.y + box.h / 2, box.x, box.w, "Views show only while your read receipts are on.", base | ATTR_DIM);
    }
    for (int k = 0; k < rows && d->scroll + k < d->count; k++) {
        const StatusViewer *v = &viewers[d->scroll + k];
        int y = box.y + 1 + k;
        int attr = d->scroll + k == d->selected ? tui_palette_attr(THEME_SLOT_SIDEBAR_SELECTED) : base;
        tui_fill((UiRect){ y, box.x + 1, 1, box.w - 2 }, attr);
        char name[128], when[32] = "";
        str_copy(name, sizeof(name), v->jid);
        if (name_of) name_of(ctx, v->jid, name, sizeof(name));
        if (v->viewed_at) clock_format_relative(v->viewed_at, use_24h, when, sizeof(when));
        if (v->viewed_at && clock_local_day(v->viewed_at) == clock_local_day(time(NULL))) {
            clock_format_time(v->viewed_at, use_24h, when, sizeof(when));
        }
        int right = tui_text_right(y, box.x + box.w - 2, 20, when, attr | ATTR_DIM);
        int heart = 0;
        if (v->reaction[0]) heart = tui_text_right(y, box.x + box.w - 3 - right, 4, v->reaction, attr) + 1;
        tui_text(y, box.x + 2, box.w - 6 - right - heart, name, attr | ATTR_BOLD);
    }
    tui_text_center(box.y + box.h - 1, box.x, box.w, " \xE2\x86\x91\xE2\x86\x93 scroll " DOT " Esc back ", tui_palette_attr(THEME_SLOT_BORDER));
}
