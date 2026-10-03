#include "clients/tui/chat_toggle_dialog.h"
#include "clients/tui/toggle_switch.h"
#include "clients/tui/tui_draw.h"
#include "clients/tui/tui_palette.h"
#include "engines/chat_match.h"
#include "utilities/str_util.h"
#include "utilities/utf8_text.h"

#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WIDTH  64
#define CTRL_A 1

void chat_toggle_dialog_open(ChatToggleDialog *d, const char *title, const char *all_label) {
    memset(d, 0, sizeof(*d));
    str_copy(d->title, sizeof(d->title), title ? title : "Chats");
    str_copy(d->all_label, sizeof(d->all_label), all_label && *all_label ? all_label : "All chats");
    text_field_init(&d->query, 60);
    d->open = 1;
}

void chat_toggle_dialog_close(ChatToggleDialog *d) {
    d->open = 0;
    d->caret.visible = 0;
}

void chat_toggle_dialog_set_all(ChatToggleDialog *d, int all) { d->all = all != 0; }
int  chat_toggle_dialog_all(const ChatToggleDialog *d) { return d->all; }

int chat_toggle_dialog_is_on(const ChatToggleDialog *d, const char *jid) {
    for (int i = 0; i < d->on_count; i++) if (strcmp(d->on[i], jid) == 0) return 1;
    return 0;
}

int chat_toggle_dialog_set_on(ChatToggleDialog *d, const char *jid) {
    if (!jid || !jid[0] || chat_toggle_dialog_is_on(d, jid)) return 1;
    if (d->on_count >= CHAT_TOGGLE_CAPACITY) return 0;
    str_copy(d->on[d->on_count++], sizeof(d->on[0]), jid);
    return 1;
}

int chat_toggle_dialog_chats(const ChatToggleDialog *d, const char **out, int max) {
    int n = d->on_count < max ? d->on_count : max;
    for (int i = 0; i < n; i++) out[i] = d->on[i];
    return n;
}

/* Flips one chat's switch. With All chats on, that switch goes off first, so the one flipped is the one meant. */
static void flip(ChatToggleDialog *d, const char *jid) {
    d->full = 0;
    if (d->all) { d->all = 0; return; }
    for (int i = 0; i < d->on_count; i++) {
        if (strcmp(d->on[i], jid) != 0) continue;
        memmove(d->on[i], d->on[i + 1], sizeof(d->on[0]) * (size_t)(d->on_count - i - 1));
        d->on_count--;
        return;
    }
    if (!chat_toggle_dialog_set_on(d, jid)) d->full = 1;
}

static void flip_all(ChatToggleDialog *d) {
    d->all = !d->all;
    d->full = 0;
}

/* Row 0 is All chats; the chats follow. */
static void flip_row(ChatToggleDialog *d, int row) {
    if (row == 0) flip_all(d);
    else if (row - 1 < d->row_count) flip(d, d->row_jid[row - 1]);
}

static void step(ChatToggleDialog *d, int delta) {
    d->selected += delta;
    if (d->selected > d->row_count) d->selected = d->row_count;
    if (d->selected < 0) d->selected = 0;
}

static PopupResult save(ChatToggleDialog *d) {
    chat_toggle_dialog_close(d);
    return POPUP_CHOSEN;
}

PopupResult chat_toggle_dialog_key(ChatToggleDialog *d, int is_key, int ch) {
    if (!is_key && ch == 27) { chat_toggle_dialog_close(d); return POPUP_CLOSED; }
    if ((!is_key && (ch == '\n' || ch == '\r')) || (is_key && ch == KEY_ENTER)) return save(d);
    if (!is_key && ch == CTRL_A) { flip_all(d); return POPUP_CHANGED; }
    if (!is_key && ch == ' ') { flip_row(d, d->selected); return POPUP_CHANGED; }
    if (is_key && ch == KEY_UP)    { step(d, -1); return POPUP_CHANGED; }
    if (is_key && ch == KEY_DOWN)  { step(d, 1); return POPUP_CHANGED; }
    if (is_key && ch == KEY_PPAGE) { step(d, -10); return POPUP_CHANGED; }
    if (is_key && ch == KEY_NPAGE) { step(d, 10); return POPUP_CHANGED; }
    if (text_field_key(&d->query, is_key, ch)) {        /* typing filters the list */
        d->selected = d->row_count > 0 ? 1 : 0;
        d->scroll = 0;
        return POPUP_CHANGED;
    }
    return POPUP_NONE;
}

PopupResult chat_toggle_dialog_click(ChatToggleDialog *d, int y, int x) {
    if (!ui_rect_contains(d->last_rect, y, x)) { chat_toggle_dialog_close(d); return POPUP_CLOSED; }
    if (ui_rect_contains(d->save_button, y, x)) return save(d);
    if (ui_rect_contains(d->all_rect, y, x)) {
        d->selected = 0;
        flip_all(d);
        return POPUP_CHANGED;
    }
    if (ui_rect_contains(d->list_rect, y, x)) {
        int row = d->scroll + (y - d->list_rect.y);
        if (row >= 0 && row < d->row_count) {
            d->selected = row + 1;
            flip_row(d, row + 1);
        }
        return POPUP_CHANGED;
    }
    return POPUP_NONE;
}

void chat_toggle_dialog_wheel(ChatToggleDialog *d, int delta) { step(d, delta); }

void chat_toggle_dialog_paste(ChatToggleDialog *d, const char *utf8) {
    text_field_paste(&d->query, utf8);
    d->selected = d->scroll = 0;
}

void chat_toggle_dialog_render(ChatToggleDialog *d, UiRect a, const Chat *chats, int count) {
    char *filter = text_field_text(&d->query);
    d->row_count = 0;
    /* Chats switched on first, so they stay in view whatever is typed; then the rest that match. */
    for (int pass = 0; pass < 2; pass++) {
        for (int i = 0; i < count && d->row_count < CHAT_TOGGLE_ROWS; i++) {
            const Chat *c = &chats[i];
            int on = chat_toggle_dialog_is_on(d, c->jid);
            if (pass == 0 ? !on : on) continue;
            if (!chat_match_searchable(c, 0)) continue;
            if (!on && !chat_match_filter(c, filter)) continue;
            str_copy(d->row_jid[d->row_count++], sizeof(d->row_jid[0]), c->jid);
        }
    }
    free(filter);
    if (d->selected > d->row_count) d->selected = d->row_count;

    int w = a.w < WIDTH ? a.w : WIDTH;
    int h = a.h < 26 ? a.h : 26;
    UiRect box = { a.y + (a.h - h) / 2, a.x + (a.w - w) / 2, h, w };
    d->last_rect = box;
    int base = tui_palette_attr(THEME_SLOT_BASE);
    int picked = tui_palette_attr(THEME_SLOT_SIDEBAR_SELECTED);
    tui_fill(box, base);
    tui_box(box, d->title, tui_palette_attr(THEME_SLOT_BORDER));

    /* All chats, on its own line above the search. */
    UiRect all = { box.y + 1, box.x + 1, 1, box.w - 2 };
    d->all_rect = all;
    int all_attr = d->selected == 0 ? picked : base;
    tui_fill(all, all_attr);
    toggle_switch_draw(all.y, all.x + 1, d->all, all_attr);
    tui_text(all.y, all.x + 2 + TOGGLE_SWITCH_COLUMNS, all.w - 3 - TOGGLE_SWITCH_COLUMNS, d->all_label, all_attr | ATTR_BOLD);

    /* The search box. */
    UiRect frame = { box.y + 2, box.x + 2, 3, box.w - 4 };
    tui_box(frame, NULL, tui_palette_attr(THEME_SLOT_ACCENT));
    UiRect field = { frame.y + 1, frame.x + 1, 1, frame.w - 2 };
    text_field_render(&d->query, field, tui_palette_attr(THEME_SLOT_COMPOSER), 1, &d->caret);
    if (text_field_length(&d->query) == 0) tui_text(field.y, field.x, field.w, "Search chats", tui_palette_attr(THEME_SLOT_COMPOSER) | ATTR_DIM);

    /* The chats, each with its switch. With All chats on they are all on, shown dimmed. */
    UiRect list = { box.y + 6, box.x + 1, box.h - 9, box.w - 2 };
    d->list_rect = list;
    int at = d->selected - 1;                                   /* the highlighted chat, or -1 on All chats */
    if (at >= 0 && at < d->scroll) d->scroll = at;
    if (at >= d->scroll + list.h) d->scroll = at - list.h + 1;
    if (at < 0) d->scroll = 0;
    if (d->row_count == 0) tui_text_center(list.y + 1, list.x, list.w, "No chats match", base | ATTR_DIM);
    for (int r = 0; r < list.h && d->scroll + r < d->row_count; r++) {
        int k = d->scroll + r;
        const Chat *c = NULL;
        for (int i = 0; i < count && !c; i++) if (strcmp(chats[i].jid, d->row_jid[k]) == 0) c = &chats[i];
        int on = d->all || chat_toggle_dialog_is_on(d, d->row_jid[k]);
        int attr = k == at ? picked : base;
        tui_fill((UiRect){ list.y + r, list.x, 1, list.w }, attr);
        toggle_switch_draw(list.y + r, list.x + 1, on, attr);
        tui_text(list.y + r, list.x + 2 + TOGGLE_SWITCH_COLUMNS, list.w - 3 - TOGGLE_SWITCH_COLUMNS,
                 c && c->name[0] ? c->name : d->row_jid[k], attr | (on && !d->all ? ATTR_BOLD : 0) | (d->all ? ATTR_DIM : 0));
    }

    /* How many are on, and Save. */
    int y = box.y + box.h - 2;
    char note[96];
    if (d->full) snprintf(note, sizeof(note), "At most %d chats one by one; use All chats for more", CHAT_TOGGLE_CAPACITY);
    else if (d->all) snprintf(note, sizeof(note), "On for every chat");
    else if (d->on_count) snprintf(note, sizeof(note), "On for %d chat%s", d->on_count, d->on_count == 1 ? "" : "s");
    else snprintf(note, sizeof(note), "Off for every chat");
    tui_text(y, box.x + 2, box.w - 16, note, d->full ? tui_palette_attr(THEME_SLOT_WARN) | ATTR_BOLD : base | ATTR_DIM);
    const char *label = "  Save  ";
    int bw = utf8_columns(label);
    tui_text(y, box.x + box.w - 2 - bw, bw, label, picked | ATTR_BOLD);
    d->save_button = (UiRect){ y, box.x + box.w - 2 - bw, 1, bw };
    tui_text_center(box.y + box.h - 1, box.x, box.w, " Space switch \xC2\xB7 Ctrl+A all \xC2\xB7 Enter save \xC2\xB7 Esc close ", tui_palette_attr(THEME_SLOT_BORDER));
}
