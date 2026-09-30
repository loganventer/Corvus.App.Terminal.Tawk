#include "clients/tui/chat_picker.h"
#include "clients/tui/tui_draw.h"
#include "clients/tui/tui_palette.h"
#include "engines/chat_match.h"
#include "utilities/str_util.h"
#include "utilities/utf8_text.h"

#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WIDTH 60
#define TICK  "\xE2\x9C\x93"   /* ✓ */
#define RING  "\xE2\x97\x8B"   /* ○ */

void chat_picker_open(ChatPicker *p, const char *title) {
    memset(p, 0, sizeof(*p));
    str_copy(p->title, sizeof(p->title), title ? title : "Forward to");
    text_field_init(&p->query, 60);
    p->open = 1;
}

void chat_picker_close(ChatPicker *p) {
    p->open = 0;
    p->caret.visible = 0;
}

int chat_picker_is_chosen(const ChatPicker *p, const char *jid) {
    for (int i = 0; i < p->chosen_count; i++) if (strcmp(p->chosen[i], jid) == 0) return 1;
    return 0;
}

int chat_picker_chosen(const ChatPicker *p, const char *out[CHAT_PICKER_MAX_CHOSEN]) {
    for (int i = 0; i < p->chosen_count; i++) out[i] = p->chosen[i];
    return p->chosen_count;
}

/* Ticks the chat, or unticks it when it already is. */
static void toggle(ChatPicker *p, const char *jid) {
    p->full = 0;
    for (int i = 0; i < p->chosen_count; i++) {
        if (strcmp(p->chosen[i], jid) != 0) continue;
        memmove(p->chosen[i], p->chosen[i + 1], sizeof(p->chosen[0]) * (size_t)(p->chosen_count - i - 1));
        p->chosen_count--;
        return;
    }
    if (p->chosen_count >= CHAT_PICKER_MAX_CHOSEN) { p->full = 1; return; }
    str_copy(p->chosen[p->chosen_count++], sizeof(p->chosen[0]), jid);
}

static void step(ChatPicker *p, int delta) {
    p->selected += delta;
    if (p->selected >= p->row_count) p->selected = p->row_count - 1;
    if (p->selected < 0) p->selected = 0;
}

/* Enter or Send: the ticked chats, or the highlighted one when none is. */
static PopupResult send(ChatPicker *p) {
    if (p->chosen_count == 0) {
        if (p->selected < 0 || p->selected >= p->row_count) return POPUP_NONE;
        toggle(p, p->row_jid[p->selected]);
    }
    p->open = 0;
    p->caret.visible = 0;
    return POPUP_CHOSEN;
}

PopupResult chat_picker_key(ChatPicker *p, int is_key, int ch) {
    if (!is_key && ch == 27) { chat_picker_close(p); return POPUP_CLOSED; }
    if ((!is_key && (ch == '\n' || ch == '\r')) || (is_key && ch == KEY_ENTER)) return send(p);
    if (!is_key && ch == ' ') {
        if (p->selected >= 0 && p->selected < p->row_count) toggle(p, p->row_jid[p->selected]);
        return POPUP_CHANGED;
    }
    if (is_key && ch == KEY_UP)   { step(p, -1); return POPUP_CHANGED; }
    if (is_key && ch == KEY_DOWN) { step(p, 1); return POPUP_CHANGED; }
    if (is_key && ch == KEY_PPAGE) { step(p, -10); return POPUP_CHANGED; }
    if (is_key && ch == KEY_NPAGE) { step(p, 10); return POPUP_CHANGED; }
    if (text_field_key(&p->query, is_key, ch)) {        /* typing filters the list */
        p->selected = p->scroll = 0;
        return POPUP_CHANGED;
    }
    return POPUP_NONE;
}

PopupResult chat_picker_click(ChatPicker *p, int y, int x) {
    if (!ui_rect_contains(p->last_rect, y, x)) { chat_picker_close(p); return POPUP_CLOSED; }
    if (ui_rect_contains(p->send_button, y, x)) return send(p);
    if (ui_rect_contains(p->list_rect, y, x)) {
        int row = p->scroll + (y - p->list_rect.y);
        if (row >= 0 && row < p->row_count) {
            p->selected = row;
            toggle(p, p->row_jid[row]);
        }
        return POPUP_CHANGED;
    }
    return POPUP_NONE;
}

void chat_picker_wheel(ChatPicker *p, int delta) { step(p, delta); }

void chat_picker_paste(ChatPicker *p, const char *utf8) {
    text_field_paste(&p->query, utf8);
    p->selected = p->scroll = 0;
}

void chat_picker_render(ChatPicker *p, UiRect a, const Chat *chats, int count) {
    char *filter = text_field_text(&p->query);
    p->row_count = 0;
    /* Chosen chats first, so they stay in view whatever is typed; then the rest that match. */
    for (int pass = 0; pass < 2; pass++) {
        for (int i = 0; i < count && p->row_count < CHAT_PICKER_ROWS; i++) {
            const Chat *c = &chats[i];
            int chosen = chat_picker_is_chosen(p, c->jid);
            if (pass == 0 ? !chosen : chosen) continue;
            if (!chosen && (!chat_match_searchable(c, 0) || !chat_match_filter(c, filter))) continue;
            str_copy(p->row_jid[p->row_count++], sizeof(p->row_jid[0]), c->jid);
        }
    }
    free(filter);
    if (p->selected >= p->row_count) p->selected = p->row_count > 0 ? p->row_count - 1 : 0;

    int w = a.w < WIDTH ? a.w : WIDTH;
    int h = a.h < 24 ? a.h : 24;
    UiRect box = { a.y + (a.h - h) / 2, a.x + (a.w - w) / 2, h, w };
    p->last_rect = box;
    int base = tui_palette_attr(THEME_SLOT_BASE);
    tui_fill(box, base);
    tui_box(box, p->title, tui_palette_attr(THEME_SLOT_BORDER));

    /* The search box. */
    UiRect frame = { box.y + 1, box.x + 2, 3, box.w - 4 };
    tui_box(frame, NULL, tui_palette_attr(THEME_SLOT_ACCENT));
    UiRect field = { frame.y + 1, frame.x + 1, 1, frame.w - 2 };
    text_field_render(&p->query, field, tui_palette_attr(THEME_SLOT_COMPOSER), 1, &p->caret);
    if (text_field_length(&p->query) == 0) tui_text(field.y, field.x, field.w, "Search chats", tui_palette_attr(THEME_SLOT_COMPOSER) | ATTR_DIM);

    /* The chats. */
    UiRect list = { box.y + 5, box.x + 1, box.h - 8, box.w - 2 };
    p->list_rect = list;
    if (p->selected < p->scroll) p->scroll = p->selected;
    if (p->selected >= p->scroll + list.h) p->scroll = p->selected - list.h + 1;
    if (p->row_count == 0) tui_text_center(list.y + 1, list.x, list.w, "No chats match", base | ATTR_DIM);
    for (int r = 0; r < list.h && p->scroll + r < p->row_count; r++) {
        int k = p->scroll + r;
        const Chat *c = NULL;
        for (int i = 0; i < count && !c; i++) if (strcmp(chats[i].jid, p->row_jid[k]) == 0) c = &chats[i];
        int chosen = chat_picker_is_chosen(p, p->row_jid[k]);
        int attr = k == p->selected ? tui_palette_attr(THEME_SLOT_SIDEBAR_SELECTED) : base;
        tui_fill((UiRect){ list.y + r, list.x, 1, list.w }, attr);
        tui_text(list.y + r, list.x + 1, 2, chosen ? TICK : RING,
                 (chosen ? tui_palette_attr(THEME_SLOT_OK) : attr) | ATTR_BOLD);
        tui_text(list.y + r, list.x + 4, list.w - 5, c && c->name[0] ? c->name : p->row_jid[k], attr | (chosen ? ATTR_BOLD : 0));
    }

    /* How many are chosen, and Send. */
    int y = box.y + box.h - 2;
    char note[96];
    if (p->full) snprintf(note, sizeof(note), "You can forward to %d chats at a time", CHAT_PICKER_MAX_CHOSEN);
    else if (p->chosen_count) snprintf(note, sizeof(note), "%d of %d chosen", p->chosen_count, CHAT_PICKER_MAX_CHOSEN);
    else snprintf(note, sizeof(note), "Space picks chats");
    tui_text(y, box.x + 2, box.w - 16, note, p->full ? tui_palette_attr(THEME_SLOT_WARN) | ATTR_BOLD : base | ATTR_DIM);
    const char *label = "  Send \xE2\x9E\xA4  ";   /* ➤ */
    int bw = utf8_columns(label);
    tui_text(y, box.x + box.w - 2 - bw, bw, label, tui_palette_attr(THEME_SLOT_SIDEBAR_SELECTED) | ATTR_BOLD);
    p->send_button = (UiRect){ y, box.x + box.w - 2 - bw, 1, bw };
    tui_text_center(box.y + box.h - 1, box.x, box.w, " Space pick \xC2\xB7 Enter send \xC2\xB7 Esc close ", tui_palette_attr(THEME_SLOT_BORDER));
}
