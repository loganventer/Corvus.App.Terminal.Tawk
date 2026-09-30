#include "clients/tui/reaction_palette.h"
#include "clients/tui/tui_draw.h"
#include "clients/tui/tui_palette.h"
#include "utilities/str_util.h"

#include <ncurses.h>
#include <stdio.h>
#include <string.h>

static const char *const EMOJI[REACTION_PALETTE_SIZE] = {
    "\xF0\x9F\x91\x8D", "\xE2\x9D\xA4\xEF\xB8\x8F", "\xF0\x9F\x98\x82", "\xF0\x9F\x98\xAE",
    "\xF0\x9F\x98\xA2", "\xF0\x9F\x99\x8F", "", "+"
};

#define REMOVE_ITEM 6
#define MORE_ITEM   7

int reaction_palette_wants_more(const ReactionPalette *p) { return p->selected == MORE_ITEM; }

void reaction_palette_open(ReactionPalette *p, const char *message_id) {
    memset(p, 0, sizeof(*p));
    str_copy(p->message_id, sizeof(p->message_id), message_id);
    p->open = 1;
}

PopupResult reaction_palette_key(ReactionPalette *p, int is_key, int ch) {
    if (!is_key && (ch == 27 || ch == 'q')) { p->open = 0; return POPUP_CLOSED; }
    if (!is_key && ch >= '1' && ch <= '0' + REACTION_PALETTE_SIZE) {
        p->selected = ch - '1';
        p->open = 0;
        return POPUP_CHOSEN;
    }
    if (is_key && ch == KEY_LEFT && p->selected > 0) p->selected--;
    else if (is_key && ch == KEY_RIGHT && p->selected < REACTION_PALETTE_SIZE - 1) p->selected++;
    else if ((!is_key && (ch == '\n' || ch == '\r' || ch == ' ')) || (is_key && ch == KEY_ENTER)) {
        p->open = 0;
        return POPUP_CHOSEN;
    }
    return POPUP_NONE;
}

PopupResult reaction_palette_click(ReactionPalette *p, int y, int x) {
    if (!ui_rect_contains(p->last_rect, y, x)) { p->open = 0; return POPUP_CLOSED; }
    for (int i = 0; i < REACTION_PALETTE_SIZE; i++) {
        if (x >= p->item_x[i] && x < p->item_x[i + 1]) {
            p->selected = i;
            p->open = 0;
            return POPUP_CHOSEN;
        }
    }
    return POPUP_NONE;
}

const char *reaction_palette_choice(const ReactionPalette *p) {
    if (p->selected == MORE_ITEM) return "";
    return EMOJI[p->selected >= 0 && p->selected < REACTION_PALETTE_SIZE ? p->selected : 0];
}

void reaction_palette_render(ReactionPalette *p, UiRect a) {
    int w = 52;
    if (w > a.w) w = a.w;
    UiRect box = { a.y + a.h - 4, a.x + (a.w - w) / 2, 3, w };
    p->last_rect = box;
    tui_box(box, "React", tui_palette_attr(THEME_SLOT_BORDER));
    int x = box.x + 2;
    for (int i = 0; i < REACTION_PALETTE_SIZE; i++) {
        p->item_x[i] = x;
        const char *label = i == REMOVE_ITEM ? "\xE2\x9C\x95" : i == MORE_ITEM ? "\xE2\x9E\x95 more" : EMOJI[i];
        char cell[32];
        snprintf(cell, sizeof(cell), " %s ", label);
        int attr = i == p->selected ? tui_palette_attr(THEME_SLOT_BADGE) | ATTR_BOLD : tui_palette_attr(THEME_SLOT_BASE);
        x += tui_text(box.y + 1, x, box.x + box.w - 1 - x, cell, attr) + 1;
    }
    p->item_x[REACTION_PALETTE_SIZE] = x;
}
