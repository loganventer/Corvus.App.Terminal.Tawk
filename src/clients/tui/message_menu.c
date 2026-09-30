#include "clients/tui/message_menu.h"
#include "clients/tui/tui_draw.h"
#include "clients/tui/tui_palette.h"

#include <ncurses.h>
#include <string.h>

#define MENU_WIDTH 28

void message_menu_open(MessageMenu *m, int message, int y, int x, const int enabled[MESSAGE_ACTION_COUNT]) {
    memset(m, 0, sizeof(*m));
    m->message = message;
    m->anchor_y = y;
    m->anchor_x = x;
    for (int a = 0; a < MESSAGE_ACTION_COUNT; a++) if (enabled[a]) m->items[m->count++] = (MessageAction)a;
    m->open = m->count > 0;
}

PopupResult message_menu_key(MessageMenu *m, int is_key, int ch) {
    if (!is_key && (ch == 27 || ch == 'q')) { m->open = 0; return POPUP_CLOSED; }
    if (is_key && ch == KEY_UP && m->selected > 0) m->selected--;
    else if (is_key && ch == KEY_DOWN && m->selected < m->count - 1) m->selected++;
    else if ((!is_key && (ch == '\n' || ch == '\r' || ch == ' ')) || (is_key && ch == KEY_ENTER)) {
        m->open = 0;
        return POPUP_CHOSEN;
    }
    return POPUP_NONE;
}

PopupResult message_menu_click(MessageMenu *m, int y, int x) {
    if (!ui_rect_contains(m->last_rect, y, x)) { m->open = 0; return POPUP_CLOSED; }
    int k = y - m->last_rect.y - 1;
    if (k < 0 || k >= m->count) return POPUP_NONE;
    m->selected = k;
    m->open = 0;
    return POPUP_CHOSEN;
}

MessageAction message_menu_choice(const MessageMenu *m) { return m->items[m->selected]; }

void message_menu_render(MessageMenu *m, UiRect b) {
    int h = m->count + 2, w = MENU_WIDTH;
    int y = m->anchor_y, x = m->anchor_x;
    if (y + h > b.y + b.h) y = b.y + b.h - h;          /* keep the menu on screen */
    if (x + w > b.x + b.w) x = b.x + b.w - w;
    if (y < b.y) y = b.y;
    if (x < b.x) x = b.x;
    UiRect box = { y, x, h, w };
    m->last_rect = box;
    int base = tui_palette_attr(THEME_SLOT_BASE);
    tui_box(box, "", tui_palette_attr(THEME_SLOT_BORDER));
    for (int i = 0; i < m->count; i++) {
        int attr = i == m->selected ? tui_palette_attr(THEME_SLOT_SIDEBAR_SELECTED) | ATTR_BOLD : base;
        tui_fill((UiRect){ box.y + 1 + i, box.x + 1, 1, box.w - 2 }, attr);
        tui_text(box.y + 1 + i, box.x + 2, box.w - 4, message_action_label(m->items[i]), attr);
    }
}
