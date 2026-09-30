#include "clients/tui/emoji_suggestions.h"
#include "clients/tui/tui_draw.h"
#include "clients/tui/tui_palette.h"

#include <stdio.h>
#include <string.h>

void emoji_suggestions_open(EmojiSuggestions *s, const int *matches, int count, int start, int end) {
    int dismissed = s->dismissed;
    int keep = s->count && s->start == start ? s->selected : 0;    /* same shortcode, still typing */
    memset(s, 0, sizeof(*s));
    s->dismissed = dismissed;
    if (count > EMOJI_SHORTCODE_MAX) count = EMOJI_SHORTCODE_MAX;
    memcpy(s->matches, matches, sizeof(int) * (size_t)(count > 0 ? count : 0));
    s->count = count > 0 ? count : 0;
    s->start = start;
    s->end = end;
    s->selected = keep < s->count ? keep : 0;
}

void emoji_suggestions_close(EmojiSuggestions *s) { s->count = 0; }

void emoji_suggestions_dismiss(EmojiSuggestions *s) {
    s->dismissed = s->count ? s->start + 1 : 0;
    s->count = 0;
}

void emoji_suggestions_move(EmojiSuggestions *s, int delta) {
    if (s->count) s->selected = (s->selected + delta + s->count) % s->count;
}

const char *emoji_suggestions_glyph(const EmojiSuggestions *s, IEmojiCatalog *catalog) {
    if (!s->count) return NULL;
    const Emoji *e = catalog->at(catalog, s->matches[s->selected]);
    return e ? e->glyph : NULL;
}

int emoji_suggestions_hit(const EmojiSuggestions *s, int y, int x) {
    for (int i = 0; i < s->count; i++) if (ui_rect_contains(s->cells[i], y, x)) return i;
    return -1;
}

void emoji_suggestions_render(EmojiSuggestions *s, IEmojiCatalog *catalog, UiRect anchor) {
    memset(s->cells, 0, sizeof(s->cells));
    if (!s->count) return;
    int base = tui_palette_attr(THEME_SLOT_COMPOSER);
    int chosen = tui_palette_attr(THEME_SLOT_SIDEBAR_SELECTED) | ATTR_BOLD;
    int y = anchor.y - 1;
    int w = anchor.w < 96 ? anchor.w : 96;
    int fit = (w - 26) / 4;                         /* room left for the name */
    if (fit < 3) fit = (w - 4) / 4;
    if (fit < 1) return;
    if (s->selected < s->first) s->first = s->selected;
    if (s->selected >= s->first + fit) s->first = s->selected - fit + 1;
    if (s->first > s->count - fit) s->first = s->count - fit > 0 ? s->count - fit : 0;
    tui_fill((UiRect){ y, anchor.x, 1, w }, base);
    tui_text(y, anchor.x, 1, s->first > 0 ? "\xE2\x80\xB9" : " ", base | ATTR_DIM);            /* ‹ */
    int x = anchor.x + 1;
    for (int i = s->first; i < s->count && i < s->first + fit; i++) {
        const Emoji *e = catalog->at(catalog, s->matches[i]);
        int attr = i == s->selected ? chosen : base;
        tui_fill((UiRect){ y, x, 1, 4 }, attr);
        tui_text(y, x + 1, 2, e ? e->glyph : "?", attr);
        s->cells[i] = (UiRect){ y, x, 1, 4 };
        x += 4;
    }
    tui_text(y, x, 1, s->first + fit < s->count ? "\xE2\x80\xBA" : " ", base | ATTR_DIM);     /* › */
    const Emoji *sel = catalog->at(catalog, s->matches[s->selected]);
    char name[128];
    snprintf(name, sizeof(name), " %s  %d/%d", sel ? sel->name : "", s->selected + 1, s->count);
    tui_text(y, x + 2, anchor.x + w - x - 2, name, base | ATTR_DIM);
}
