#include "clients/tui/search_overlay.h"
#include "clients/tui/tui_draw.h"
#include "clients/tui/tui_palette.h"
#include "utilities/clock_util.h"
#include "utilities/str_util.h"

#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ROWS_PER_RESULT 2

void search_overlay_init(SearchOverlay *o) { memset(o, 0, sizeof(*o)); }

void search_overlay_close(SearchOverlay *o) {
    message_array_free(o->results, o->count);
    o->results = NULL;
    o->count = o->selected = o->scroll = 0;
    o->open = 0;
}

void search_overlay_open(SearchOverlay *o, const char *query) {
    search_overlay_close(o);
    str_copy(o->query, sizeof(o->query), query ? query : "");
    o->open = 1;
}

void search_overlay_set_results(SearchOverlay *o, Message *results, int count) {
    message_array_free(o->results, o->count);
    o->results = results;
    o->count = count;
    o->selected = o->scroll = 0;
}

const Message *search_overlay_selected(const SearchOverlay *o) {
    return (o->selected >= 0 && o->selected < o->count) ? &o->results[o->selected] : NULL;
}

PopupResult search_overlay_key(SearchOverlay *o, int is_key, int ch) {
    size_t len = strlen(o->query);
    if (!is_key && ch == 27) { search_overlay_close(o); return POPUP_CLOSED; }
    if ((!is_key && (ch == '\n' || ch == '\r')) || (is_key && ch == KEY_ENTER)) {
        return o->count ? POPUP_CHOSEN : POPUP_NONE;
    }
    if (is_key) {
        if (ch == KEY_UP && o->selected > 0) o->selected--;
        else if (ch == KEY_DOWN && o->selected < o->count - 1) o->selected++;
        else if (ch == KEY_BACKSPACE && len) { o->query[len - 1] = '\0'; return POPUP_CHANGED; }
        return POPUP_NONE;
    }
    if (ch == 127 || ch == 8) {
        while (len > 0 && ((unsigned char)o->query[len - 1] & 0xC0) == 0x80) len--;
        if (len) { o->query[len - 1] = '\0'; return POPUP_CHANGED; }
        return POPUP_NONE;
    }
    if (ch >= 32 && ch != 127) {
        char utf8[8];
        wchar_t wc = (wchar_t)ch;
        int n = wctomb(utf8, wc);
        if (n > 0 && len + (size_t)n + 1 < sizeof(o->query)) {
            memcpy(o->query + len, utf8, (size_t)n);
            o->query[len + (size_t)n] = '\0';
            return POPUP_CHANGED;
        }
    }
    return POPUP_NONE;
}

PopupResult search_overlay_click(SearchOverlay *o, int y, int x) {
    if (!ui_rect_contains(o->last_rect, y, x)) return POPUP_NONE;
    int k = y - o->last_rect.y;
    if (k < 0 || k >= SEARCH_OVERLAY_ROWS || o->row_item[k] < 0) return POPUP_NONE;
    o->selected = o->row_item[k];
    return POPUP_CHOSEN;
}

void search_overlay_wheel(SearchOverlay *o, int delta) {
    o->selected += delta;
    if (o->selected >= o->count) o->selected = o->count - 1;
    if (o->selected < 0) o->selected = 0;
}

void search_overlay_render(SearchOverlay *o, UiRect r, const NameResolver *names, const MessageFormatter *formatter, int use_24h) {
    o->last_rect = r;
    for (int i = 0; i < SEARCH_OVERLAY_ROWS; i++) o->row_item[i] = -1;
    int base = tui_palette_attr(THEME_SLOT_BASE);
    tui_box(r, "\xF0\x9F\x94\x8D Search messages", tui_palette_attr(THEME_SLOT_BORDER));
    tui_fill((UiRect){ r.y + 1, r.x + 1, r.h - 2, r.w - 2 }, base);
    char field[200];
    snprintf(field, sizeof(field), " %s", o->query);
    tui_fill((UiRect){ r.y + 1, r.x + 1, 1, r.w - 2 }, tui_palette_attr(THEME_SLOT_COMPOSER));
    int used = tui_text(r.y + 1, r.x + 1, r.w - 2, field, tui_palette_attr(THEME_SLOT_COMPOSER) | ATTR_BOLD);
    o->caret = (TextCaret){ 1, r.y + 1, r.x + 1 + used };

    UiRect list = { r.y + 3, r.x + 1, r.h - 5, r.w - 2 };
    if (!o->query[0]) {
        tui_text_center(list.y + 1, r.x, r.w, "Type to search every chat", base | ATTR_DIM);
    } else if (!o->count) {
        tui_text_center(list.y + 1, r.x, r.w, "No messages match", base | ATTR_DIM);
    }
    int slots = list.h / ROWS_PER_RESULT;
    if (slots < 1) slots = 1;
    if (o->selected < o->scroll) o->scroll = o->selected;
    if (o->selected >= o->scroll + slots) o->scroll = o->selected - slots + 1;
    for (int k = 0; k < slots && o->scroll + k < o->count; k++) {
        int i = o->scroll + k;
        const Message *m = &o->results[i];
        int y = list.y + k * ROWS_PER_RESULT;
        int attr = i == o->selected ? tui_palette_attr(THEME_SLOT_SIDEBAR_SELECTED) : base;
        tui_fill((UiRect){ y, list.x, ROWS_PER_RESULT, list.w }, attr);
        char chat[128], when[32], head[300];
        names->resolve(names->ctx, m->chat_jid, chat, sizeof(chat));
        clock_format_relative(m->timestamp, use_24h, when, sizeof(when));
        snprintf(head, sizeof(head), " %s%s%s", chat, m->from_me ? "  \xC2\xB7 You" : "",
                 (!m->from_me && m->sender_name[0] && strcmp(m->sender_name, chat)) ? "" : "");
        int wc = tui_text_right(y, list.x + list.w - 1, 16, when, attr | ATTR_DIM);
        tui_text(y, list.x, list.w - wc - 2, head, attr | ATTR_BOLD);
        char snippet[300];
        if (formatter) formatter->preview(formatter->ctx, m, snippet, sizeof(snippet));
        else message_preview(m, snippet, sizeof(snippet));
        char line[320];
        snprintf(line, sizeof(line), "   %s", snippet);
        tui_text(y + 1, list.x, list.w - 1, line, attr);
        for (int row = 0; row < ROWS_PER_RESULT; row++) {
            if (y + row - r.y < SEARCH_OVERLAY_ROWS) o->row_item[y + row - r.y] = i;
        }
    }
    tui_text_center(r.y + r.h - 2, r.x, r.w, "\xE2\x86\x91\xE2\x86\x93 choose \xC2\xB7 Enter open \xC2\xB7 Esc close",
                    tui_palette_attr(THEME_SLOT_DIM));
}
