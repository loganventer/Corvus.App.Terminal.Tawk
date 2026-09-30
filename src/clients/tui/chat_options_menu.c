#include "clients/tui/chat_options_menu.h"
#include "clients/tui/tui_draw.h"
#include "clients/tui/tui_palette.h"
#include "utilities/str_util.h"

#include <ncurses.h>
#include <string.h>

static void add(ChatOptionsMenu *m, ChatOption option) {
    if (m->count < CHAT_OPTION_COUNT) m->items[m->count++] = option;
}

void chat_options_menu_open(ChatOptionsMenu *m, const Chat *c) {
    memset(m, 0, sizeof(*m));
    str_copy(m->jid, sizeof(m->jid), c->jid);
    str_copy(m->title, sizeof(m->title), c->name);
    if (c->is_muted) add(m, CHAT_OPTION_UNMUTE);
    else { add(m, CHAT_OPTION_MUTE_8H); add(m, CHAT_OPTION_MUTE_WEEK); add(m, CHAT_OPTION_MUTE_ALWAYS); }
    add(m, c->is_pinned ? CHAT_OPTION_UNPIN : CHAT_OPTION_PIN);
    add(m, c->is_archived > 0 ? CHAT_OPTION_UNARCHIVE : CHAT_OPTION_ARCHIVE);
    add(m, CHAT_OPTION_THEME);
    add(m, CHAT_OPTION_TONE_CHOOSE);
    add(m, strcmp(c->tone, "none") == 0 ? CHAT_OPTION_TONE_DEFAULT : CHAT_OPTION_TONE_NONE);
    if (c->tone[0] && strcmp(c->tone, "none") != 0) add(m, CHAT_OPTION_TONE_DEFAULT);
    if (c->has_draft) add(m, CHAT_OPTION_CLEAR_DRAFT);
    add(m, c->soft_locked ? CHAT_OPTION_SOFT_UNLOCK : CHAT_OPTION_SOFT_LOCK);
    add(m, CHAT_OPTION_INFO);
    add(m, CHAT_OPTION_DELETE_CHAT);
    m->open = 1;
}

PopupResult chat_options_menu_key(ChatOptionsMenu *m, int is_key, int ch) {
    if (!is_key && (ch == 27 || ch == 'q')) { m->open = 0; return POPUP_CLOSED; }
    if (is_key && ch == KEY_UP && m->selected > 0) m->selected--;
    else if (is_key && ch == KEY_DOWN && m->selected < m->count - 1) m->selected++;
    else if ((!is_key && (ch == '\n' || ch == '\r' || ch == ' ')) || (is_key && (ch == KEY_ENTER || ch == KEY_RIGHT))) {
        m->open = 0;
        return POPUP_CHOSEN;
    }
    return POPUP_NONE;
}

PopupResult chat_options_menu_click(ChatOptionsMenu *m, int y, int x) {
    if (!ui_rect_contains(m->last_rect, y, x)) { m->open = 0; return POPUP_CLOSED; }
    int k = y - m->last_rect.y;
    if (k < 0 || k >= 64 || m->row_item[k] < 0) return POPUP_NONE;
    m->selected = m->row_item[k];
    m->open = 0;
    return POPUP_CHOSEN;
}

ChatOption chat_options_menu_choice(const ChatOptionsMenu *m) {
    return m->items[m->selected < m->count ? m->selected : 0];
}

void chat_options_menu_render(ChatOptionsMenu *m, UiRect a) {
    int w = a.w < 48 ? a.w : 48;
    int h = m->count + 4;
    if (h > a.h) h = a.h;
    UiRect box = { a.y + (a.h - h) / 2, a.x + (a.w - w) / 2, h, w };
    m->last_rect = box;
    for (int i = 0; i < 64; i++) m->row_item[i] = -1;
    int base = tui_palette_attr(THEME_SLOT_BASE);
    tui_box(box, m->title, tui_palette_attr(THEME_SLOT_BORDER));
    tui_fill((UiRect){ box.y + 1, box.x + 1, box.h - 2, box.w - 2 }, base);
    for (int i = 0; i < m->count && i < box.h - 3; i++) {
        int y = box.y + 1 + i;
        int attr = i == m->selected ? tui_palette_attr(THEME_SLOT_SIDEBAR_SELECTED) | ATTR_BOLD : base;
        tui_fill((UiRect){ y, box.x + 1, 1, box.w - 2 }, attr);
        tui_text(y, box.x + 2, box.w - 4, chat_option_label(m->items[i]), attr);
        if (y - box.y < 64) m->row_item[y - box.y] = i;
    }
    tui_text_center(box.y + box.h - 2, box.x, box.w, "Enter choose \xC2\xB7 Esc close", tui_palette_attr(THEME_SLOT_DIM));
}
