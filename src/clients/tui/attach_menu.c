#include "clients/tui/attach_menu.h"
#include "clients/tui/tui_draw.h"
#include "clients/tui/tui_palette.h"

#include <ncurses.h>
#include <string.h>

#define WIDTH 34

void attach_menu_open(AttachMenu *m, int has_camera) {
    memset(m, 0, sizeof(*m));
    m->has_camera = has_camera;
    m->selected = has_camera ? ATTACH_CHOICE_PHOTO : ATTACH_CHOICE_FILE;
    m->open = 1;
}

PopupResult attach_menu_key(AttachMenu *m, int is_key, int ch) {
    if (!is_key && (ch == 27 || ch == 'q')) { m->open = 0; return POPUP_CLOSED; }
    if (is_key && ch == KEY_UP && m->selected > 0) m->selected--;
    else if (is_key && ch == KEY_DOWN && m->selected < ATTACH_CHOICE_COUNT - 1) m->selected++;
    else if ((!is_key && (ch == '\n' || ch == '\r' || ch == ' ')) || (is_key && (ch == KEY_ENTER || ch == KEY_RIGHT))) {
        m->open = 0;
        return POPUP_CHOSEN;
    }
    return POPUP_NONE;
}

PopupResult attach_menu_click(AttachMenu *m, int y, int x) {
    if (!ui_rect_contains(m->last_rect, y, x)) { m->open = 0; return POPUP_CLOSED; }
    int item = y - m->last_rect.y - 1;
    if (item < 0 || item >= ATTACH_CHOICE_COUNT) return POPUP_NONE;
    m->selected = item;
    m->open = 0;
    return POPUP_CHOSEN;
}

AttachChoice attach_menu_choice(const AttachMenu *m) { return (AttachChoice)m->selected; }

void attach_menu_render(AttachMenu *m, UiRect a) {
    int w = a.w < WIDTH ? a.w : WIDTH;
    int h = ATTACH_CHOICE_COUNT + 2;
    if (h > a.h) h = a.h;
    UiRect box = { a.y + a.h - h, a.x + a.w - w, h, w };
    m->last_rect = box;
    int base = tui_palette_attr(THEME_SLOT_BASE);
    tui_box(box, NULL, tui_palette_attr(THEME_SLOT_BORDER));
    tui_fill((UiRect){ box.y + 1, box.x + 1, box.h - 2, box.w - 2 }, base);
    for (int i = 0; i < ATTACH_CHOICE_COUNT && i < box.h - 2; i++) {
        int y = box.y + 1 + i;
        int attr = i == m->selected ? tui_palette_attr(THEME_SLOT_SIDEBAR_SELECTED) | ATTR_BOLD : base;
        int unusable = i == ATTACH_CHOICE_PHOTO && !m->has_camera;
        if (unusable) attr |= ATTR_DIM;
        tui_fill((UiRect){ y, box.x + 1, 1, box.w - 2 }, attr);
        int used = tui_text(y, box.x + 2, box.w - 4, attach_choice_label((AttachChoice)i), attr);
        if (unusable) tui_text(y, box.x + 2 + used, box.w - 4 - used, " (no camera)", attr);
    }
}
