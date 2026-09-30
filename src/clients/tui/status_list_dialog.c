#include "clients/tui/status_list_dialog.h"
#include "clients/tui/tui_draw.h"
#include "clients/tui/tui_palette.h"
#include "utilities/clock_util.h"

#include <ncurses.h>
#include <stdio.h>
#include <string.h>

#define WIDTH 60
#define DOT   "\xC2\xB7"

void status_list_dialog_open(StatusListDialog *d) {
    memset(d, 0, sizeof(*d));
    d->open = 1;
}

static void clamp(StatusListDialog *d) {
    if (d->selected >= d->count) d->selected = d->count - 1;
    if (d->selected < 0) d->selected = 0;
}

PopupResult status_list_dialog_key(StatusListDialog *d, int is_key, int ch, int *post) {
    *post = 0;
    if (!is_key && (ch == 27 || ch == 'q')) { d->open = 0; return POPUP_CLOSED; }
    if (!is_key && (ch == 'n' || ch == '+')) { *post = 1; return POPUP_CHANGED; }
    if ((!is_key && ch == '\t') || (is_key && ch == KEY_BTAB)) {
        d->archived = !d->archived;
        d->selected = d->scroll = 0;
        return POPUP_CHANGED;
    }
    if (is_key && ch == KEY_UP) d->selected--;
    else if (is_key && ch == KEY_DOWN) d->selected++;
    else if (is_key && ch == KEY_HOME) d->selected = 0;
    else if (is_key && ch == KEY_END) d->selected = d->count - 1;
    else if ((!is_key && (ch == '\n' || ch == '\r' || ch == ' ')) || (is_key && (ch == KEY_ENTER || ch == KEY_RIGHT))) {
        return d->count > 0 ? POPUP_CHOSEN : POPUP_NONE;
    } else {
        return POPUP_NONE;
    }
    clamp(d);
    return POPUP_CHANGED;
}

PopupResult status_list_dialog_click(StatusListDialog *d, int y, int x, int *post) {
    *post = 0;
    if (!ui_rect_contains(d->last_rect, y, x)) { d->open = 0; return POPUP_CLOSED; }
    if (ui_rect_contains(d->post_button, y, x)) { *post = 1; return POPUP_CHANGED; }
    for (int t = 0; t < 2; t++) {
        if (ui_rect_contains(d->tabs[t], y, x) && d->archived != t) {
            d->archived = t;
            d->selected = d->scroll = 0;
            return POPUP_CHANGED;
        }
    }
    for (int i = 0; i < d->row_count; i++) {
        if (ui_rect_contains(d->rows[i], y, x)) { d->selected = d->row_position[i]; return POPUP_CHOSEN; }
    }
    return POPUP_NONE;
}

int status_list_dialog_choice(const StatusListDialog *d) {
    return d->selected >= 0 && d->selected < d->count ? d->order[d->selected] : -1;
}

void status_list_dialog_wheel(StatusListDialog *d, int delta) {
    d->selected += delta;
    clamp(d);
}

/* A heading line between the groups. */
static int heading(int y, UiRect box, const char *text) {
    tui_text(y, box.x + 2, box.w - 4, text, tui_palette_attr(THEME_SLOT_ACCENT) | ATTR_BOLD);
    return y + 1;
}

void status_list_dialog_render(StatusListDialog *d, UiRect a, const StatusAuthor *authors, int count,
                               const char *(*name_of)(void *ctx, const StatusAuthor *author), void *ctx, int use_24h) {
    d->count = count < STATUS_LIST_ROWS ? count : STATUS_LIST_ROWS;
    clamp(d);
    int w = a.w < WIDTH ? a.w : WIDTH;
    int h = a.h - 2 < 24 ? a.h - 2 : 24;
    if (h < 8) h = a.h;
    UiRect box = { a.y + (a.h - h) / 2, a.x + (a.w - w) / 2, h, w };
    d->last_rect = box;
    int base = tui_palette_attr(THEME_SLOT_BASE);
    tui_fill(box, base);
    tui_box(box, d->archived ? "Status archive" : "Status", tui_palette_attr(THEME_SLOT_BORDER));
    int tx = box.x + 2;
    static const char *const TABS[] = { " Recent ", " Archive " };
    for (int t = 0; t < 2; t++) {
        int attr = t == d->archived ? tui_palette_attr(THEME_SLOT_SIDEBAR_SELECTED) | ATTR_BOLD : base | ATTR_DIM;
        int used = tui_text(box.y + 1, tx, box.w / 2, TABS[t], attr);
        d->tabs[t] = (UiRect){ box.y + 1, tx, 1, used };
        tx += used + 1;
    }

    const char *post = "[ + New status ]";
    int pw = tui_text_right(box.y + 1, box.x + box.w - 2, box.w - 4, post, tui_palette_attr(THEME_SLOT_ACCENT) | ATTR_BOLD);
    d->post_button = (UiRect){ box.y + 1, box.x + box.w - 2 - pw, 1, pw };

    /* Rows in groups: yours, then unseen, then seen. The list scrolls so the
     * selected row stays in view; headings count as rows. */
    int *order = d->order, groups[STATUS_LIST_ROWS], n = 0;
    for (int pass = 0; pass < 3; pass++) {
        for (int i = 0; i < count && n < STATUS_LIST_ROWS; i++) {
            const StatusAuthor *au = &authors[i];
            int group = au->from_me ? 0 : au->unviewed > 0 && !d->archived ? 1 : 2;
            if (group == pass) { order[n] = i; groups[n] = group; n++; }
        }
    }
    int sel_pos = d->selected;
    int visible = (box.h - 5) / 2;
    if (visible < 1) visible = 1;
    if (sel_pos < d->scroll) d->scroll = sel_pos;
    if (sel_pos >= d->scroll + visible) d->scroll = sel_pos - visible + 1;

    d->row_count = 0;
    int y = box.y + 3;
    if (n == 0 && d->archived) {
        tui_text_center(y + 1, box.x, box.w, "No older statuses kept yet.", base | ATTR_DIM);
        tui_text_center(y + 2, box.x, box.w, "Statuses move here after a day (Keep statuses in Settings).", base | ATTR_DIM);
    } else if (n == 0) {
        tui_text_center(y + 1, box.x, box.w, "No statuses from the last day.", base | ATTR_DIM);
        tui_text_center(y + 2, box.x, box.w, "Press + to post one, or Tab for older ones.", base | ATTR_DIM);
    }
    int last_group = -1;
    const char *const HEADINGS[] = { "My status", "Recent updates", d->archived ? "Older updates" : "Viewed updates" };
    for (int k = d->scroll; k < n && y < box.y + box.h - 2; k++) {
        const StatusAuthor *au = &authors[order[k]];
        if (groups[k] != last_group) {
            if (groups[k] != 0 || k != d->scroll) y = heading(y, box, HEADINGS[groups[k]]);
            last_group = groups[k];
            if (y >= box.y + box.h - 2) break;
        }
        int selected = k == d->selected;
        int attr = selected ? tui_palette_attr(THEME_SLOT_SIDEBAR_SELECTED) : base;
        UiRect row = { y, box.x + 1, 1, box.w - 2 };
        tui_fill(row, attr);
        const char *ring = au->unviewed > 0 && !au->from_me ? "\xE2\x97\x8F " : "\xE2\x97\x8B ";   /* ● unseen, ○ seen */
        int used = tui_text(y, box.x + 2, 2, ring, (au->unviewed && !au->from_me ? tui_palette_attr(THEME_SLOT_OK) : attr) | ATTR_BOLD);
        const char *name = au->from_me ? "My status" : name_of ? name_of(ctx, au) : au->name;
        char when[32], right[64];
        clock_format_relative(au->latest, use_24h, when, sizeof(when));
        snprintf(right, sizeof(right), "%d " DOT " %s", au->count, when);
        int rw = tui_text_right(y, box.x + box.w - 2, 24, right, attr | ATTR_DIM);
        tui_text(y, box.x + 2 + used, box.w - 6 - used - rw, name && *name ? name : au->jid, attr | (selected ? ATTR_BOLD : 0));
        if (d->row_count < STATUS_LIST_ROWS) {
            d->rows[d->row_count] = row;
            d->row_position[d->row_count++] = k;
        }
        y++;
    }
    tui_text_center(box.y + box.h - 1, box.x, box.w, " Enter view " DOT " Tab archive " DOT " + new " DOT " Esc close ", tui_palette_attr(THEME_SLOT_BORDER));
}
