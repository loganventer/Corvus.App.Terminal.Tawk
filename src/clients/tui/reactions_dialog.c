#include "clients/tui/reactions_dialog.h"
#include "clients/tui/tui_draw.h"
#include "clients/tui/tui_palette.h"
#include "utilities/str_util.h"

#include <ncurses.h>
#include <stdio.h>
#include <string.h>

#define WIDTH 48
#define DOT   "\xC2\xB7"

void reactions_dialog_open(ReactionsDialog *d, const char *message_id) {
    memset(d, 0, sizeof(*d));
    d->open = 1;
    str_copy(d->message_id, sizeof(d->message_id), message_id);
}

static void clamp(ReactionsDialog *d) {
    if (d->selected >= d->count) d->selected = d->count - 1;
    if (d->selected < 0) d->selected = 0;
}

PopupResult reactions_dialog_key(ReactionsDialog *d, int is_key, int ch) {
    if ((!is_key && (ch == 27 || ch == 'q')) || (is_key && ch == KEY_LEFT)) { d->open = 0; return POPUP_CLOSED; }
    if (is_key && ch == KEY_UP) d->selected--;
    else if (is_key && ch == KEY_DOWN) d->selected++;
    else if (is_key && ch == KEY_HOME) d->selected = 0;
    else if (is_key && ch == KEY_END) d->selected = d->count - 1;
    else return POPUP_NONE;
    clamp(d);
    return POPUP_CHANGED;
}

PopupResult reactions_dialog_click(ReactionsDialog *d, int y, int x) {
    if (!ui_rect_contains(d->last_rect, y, x)) { d->open = 0; return POPUP_CLOSED; }
    return POPUP_NONE;
}

void reactions_dialog_wheel(ReactionsDialog *d, int delta) {
    d->selected += delta;
    clamp(d);
}

/* "Reactions 5 · 👍 3  ❤ 2": the total, then each emoji with its count. */
static void title_of(const Reaction *items, int count, char *out, size_t size) {
    ReactionGroup groups[REACTION_GROUPING_MAX];
    int n = reaction_grouping_groups(items, count, groups, REACTION_GROUPING_MAX);
    int used = snprintf(out, size, "Reactions %d", count);
    for (int g = 0; g < n && used > 0 && (size_t)used < size; g++) {
        used += snprintf(out + used, size - (size_t)used, "%s%s %d", g ? "  " : " " DOT " ", groups[g].emoji, groups[g].count);
    }
}

void reactions_dialog_render(ReactionsDialog *d, UiRect a, const Reaction *items, int count) {
    Reaction sorted[REACTION_GROUPING_MAX];
    if (count > REACTION_GROUPING_MAX) count = REACTION_GROUPING_MAX;
    if (count < 0) count = 0;
    memcpy(sorted, items, sizeof(Reaction) * (size_t)count);
    reaction_grouping_sort(sorted, count);
    d->count = count;
    clamp(d);

    int w = a.w < WIDTH ? a.w : WIDTH;
    int h = count + 3;                              /* grows with the list, up to the screen */
    if (h < 6) h = 6;
    if (h > a.h - 4) h = a.h - 4;
    if (h < 6) h = a.h;
    UiRect box = { a.y + (a.h - h) / 2, a.x + (a.w - w) / 2, h, w };
    d->last_rect = box;
    int base = tui_palette_attr(THEME_SLOT_BASE);
    char title[160];
    title_of(sorted, count, title, sizeof(title));
    tui_fill(box, base);
    tui_box(box, title, tui_palette_attr(THEME_SLOT_BORDER));

    int rows = box.h - 3;
    if (d->selected < d->scroll) d->scroll = d->selected;
    if (d->selected >= d->scroll + rows) d->scroll = d->selected - rows + 1;
    if (count == 0) tui_text_center(box.y + box.h / 2 - 1, box.x, box.w, "No reactions.", base | ATTR_DIM);
    for (int k = 0; k < rows && d->scroll + k < count; k++) {
        const Reaction *r = &sorted[d->scroll + k];
        int y = box.y + 1 + k;
        int attr = d->scroll + k == d->selected ? tui_palette_attr(THEME_SLOT_SIDEBAR_SELECTED) : base;
        tui_fill((UiRect){ y, box.x + 1, 1, box.w - 2 }, attr);
        int emoji = tui_text_right(y, box.x + box.w - 2, 8, r->emoji, attr);
        tui_text(y, box.x + 2, box.w - 6 - emoji, r->name[0] ? r->name : r->sender, attr | ATTR_BOLD);
    }
    tui_text_center(box.y + box.h - 1, box.x, box.w, " \xE2\x86\x91\xE2\x86\x93 scroll " DOT " Esc back ", tui_palette_attr(THEME_SLOT_BORDER));
}
