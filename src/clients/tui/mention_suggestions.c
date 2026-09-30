#include "clients/tui/mention_suggestions.h"
#include "clients/tui/tui_draw.h"
#include "clients/tui/tui_palette.h"

#include <stdio.h>
#include <string.h>

void mention_suggestions_open(MentionSuggestions *s, const MentionCandidate *items, int count, int start, int end) {
    if (count > MENTION_SUGGESTIONS_MAX) count = MENTION_SUGGESTIONS_MAX;
    int same = s->count && s->start == start;
    memcpy(s->items, items, sizeof(MentionCandidate) * (size_t)count);
    s->count = count;
    s->start = start;
    s->end = end;
    if (!same || s->selected >= count) s->selected = 0;
}

void mention_suggestions_close(MentionSuggestions *s) { s->count = 0; }

void mention_suggestions_dismiss(MentionSuggestions *s) {
    s->dismissed = s->start + 1;
    s->count = 0;
}

void mention_suggestions_move(MentionSuggestions *s, int delta) {
    if (!s->count) return;
    s->selected = (s->selected + delta + s->count) % s->count;
}

const MentionCandidate *mention_suggestions_selected(const MentionSuggestions *s) {
    return s->count ? &s->items[s->selected] : NULL;
}

int mention_suggestions_hit(const MentionSuggestions *s, int y, int x) {
    for (int i = 0; i < s->count; i++) if (ui_rect_contains(s->rows[i], y, x)) return i;
    return -1;
}

void mention_suggestions_render(MentionSuggestions *s, UiRect anchor) {
    if (!s->count) return;
    int w = anchor.w < 40 ? anchor.w : 40;
    int y0 = anchor.y - s->count;
    for (int i = 0; i < s->count; i++) {
        int attr = i == s->selected ? tui_palette_attr(THEME_SLOT_SIDEBAR_SELECTED) | ATTR_BOLD : tui_palette_attr(THEME_SLOT_COMPOSER);
        UiRect row = { y0 + i, anchor.x, 1, w };
        tui_fill(row, attr);
        char line[160];
        snprintf(line, sizeof(line), " @%s", s->items[i].name);
        tui_text(row.y, row.x, row.w, line, attr);
        s->rows[i] = row;
    }
}
