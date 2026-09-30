#include "clients/tui/theme_picker_overlay.h"
#include "clients/tui/tui_draw.h"
#include "clients/tui/tui_palette.h"
#include "utilities/str_util.h"

#include <ncurses.h>
#include <stdio.h>
#include <string.h>

static int positions(IThemeRepository *themes) { return themes->count(themes) + 1; }

void theme_picker_overlay_open(ThemePickerOverlay *p, IThemeRepository *themes,
                               const char *jid, const char *chat_name, const char *current_id) {
    memset(p, 0, sizeof(*p));
    str_copy(p->jid, sizeof(p->jid), jid);
    snprintf(p->title, sizeof(p->title), "\xF0\x9F\x8E\xA8 Theme for %s", chat_name ? chat_name : "this chat");
    int index = (current_id && current_id[0]) ? themes->index_of(themes, current_id) : -1;
    p->position = p->original = index >= 0 ? index + 1 : 0;
    p->open = 1;
}

PopupResult theme_picker_overlay_key(ThemePickerOverlay *p, IThemeRepository *themes, int is_key, int ch) {
    int n = positions(themes);
    if (!is_key && ch == 27) { p->position = p->original; p->open = 0; return POPUP_CLOSED; }
    if ((!is_key && (ch == '\n' || ch == '\r' || ch == ' ')) || (is_key && ch == KEY_ENTER)) { p->open = 0; return POPUP_CHOSEN; }
    int before = p->position;
    if (is_key && ch == KEY_UP) p->position--;
    else if (is_key && ch == KEY_DOWN) p->position++;
    else if (is_key && ch == KEY_PPAGE) p->position -= 10;
    else if (is_key && ch == KEY_NPAGE) p->position += 10;
    else if (is_key && ch == KEY_HOME) p->position = 0;
    else if (is_key && ch == KEY_END) p->position = n - 1;
    if (p->position < 0) p->position = 0;
    if (p->position >= n) p->position = n - 1;
    return p->position != before ? POPUP_CHANGED : POPUP_NONE;
}

PopupResult theme_picker_overlay_click(ThemePickerOverlay *p, int y, int x) {
    if (!ui_rect_contains(p->last_rect, y, x)) return POPUP_NONE;
    int k = y - p->last_rect.y;
    if (k < 0 || k >= THEME_PICKER_ROWS || p->row_item[k] < 0) return POPUP_NONE;
    if (p->row_item[k] == p->position) { p->open = 0; return POPUP_CHOSEN; }
    p->position = p->row_item[k];
    return POPUP_CHANGED;
}

void theme_picker_overlay_wheel(ThemePickerOverlay *p, IThemeRepository *themes, int delta) {
    int n = positions(themes);
    p->position += delta;
    if (p->position < 0) p->position = 0;
    if (p->position >= n) p->position = n - 1;
}

const Theme *theme_picker_overlay_selected(const ThemePickerOverlay *p, IThemeRepository *themes) {
    return p->position == 0 ? NULL : themes->at(themes, p->position - 1);
}

void theme_picker_overlay_render(ThemePickerOverlay *p, UiRect r, IThemeRepository *themes) {
    p->last_rect = r;
    for (int i = 0; i < THEME_PICKER_ROWS; i++) p->row_item[i] = -1;
    int base = tui_palette_attr(THEME_SLOT_BASE);
    tui_box(r, p->title, tui_palette_attr(THEME_SLOT_BORDER));
    tui_fill((UiRect){ r.y + 1, r.x + 1, r.h - 2, r.w - 2 }, base);
    UiRect list = { r.y + 1, r.x + 1, r.h - 3, r.w - 2 };
    int n = positions(themes);
    if (p->position < p->scroll) p->scroll = p->position;
    if (p->position >= p->scroll + list.h) p->scroll = p->position - list.h + 1;
    for (int k = 0; k < list.h && p->scroll + k < n; k++) {
        int pos = p->scroll + k;
        int attr = pos == p->position ? tui_palette_attr(THEME_SLOT_SIDEBAR_SELECTED) | ATTR_BOLD : base;
        tui_fill((UiRect){ list.y + k, list.x, 1, list.w }, attr);
        char line[256];
        if (pos == 0) snprintf(line, sizeof(line), " %s App theme (no chat theme)", p->original == 0 ? "\xE2\x9C\x93" : " ");
        else {
            const Theme *t = themes->at(themes, pos - 1);
            snprintf(line, sizeof(line), " %s %-22s %s", p->original == pos ? "\xE2\x9C\x93" : " ", t->name, t->description);
        }
        tui_text(list.y + k, list.x, list.w, line, attr);
        if (list.y + k - r.y < THEME_PICKER_ROWS) p->row_item[list.y + k - r.y] = pos;
    }
    tui_text_center(r.y + r.h - 2, r.x, r.w, "\xE2\x86\x91\xE2\x86\x93 preview on the chat \xC2\xB7 Enter apply \xC2\xB7 Esc cancel",
                    tui_palette_attr(THEME_SLOT_DIM));
}
