#include "clients/tui/command_suggestions.h"
#include "clients/tui/tui_draw.h"
#include "clients/tui/tui_palette.h"

#include <ncurses.h>
#include <stdio.h>
#include <string.h>

int command_suggestions_update(CommandSuggestions *s, const SlashCommand *all, int total, const char *typed) {
    s->count = 0;
    if (!typed || typed[0] != '/' || typed[1] == '/' || strchr(typed, ' ')) return 0;
    const char *prefix = typed + 1;
    size_t n = strlen(prefix);
    for (int i = 0; i < total && s->count < COMMAND_SUGGESTIONS_MAX; i++) {
        if (strncmp(all[i].name, prefix, n) == 0) s->matches[s->count++] = i;
    }
    if (s->selected >= s->count) s->selected = s->count ? s->count - 1 : 0;
    return s->count;
}

void command_suggestions_move(CommandSuggestions *s, int delta) {
    if (!s->count) return;
    s->selected = (s->selected + delta + s->count) % s->count;
}

const SlashCommand *command_suggestions_selected(const CommandSuggestions *s, const SlashCommand *all) {
    return s->count ? &all[s->matches[s->selected]] : NULL;
}

void command_suggestions_render(const CommandSuggestions *s, const SlashCommand *all, UiRect anchor) {
    if (!s->count) return;
    int rows = s->count > 8 ? 8 : s->count;
    int first = s->selected >= rows ? s->selected - rows + 1 : 0;
    int w = anchor.w < 64 ? anchor.w : 64;
    int y0 = anchor.y - rows;
    for (int k = 0; k < rows; k++) {
        int i = first + k;
        const SlashCommand *c = &all[s->matches[i]];
        int attr = i == s->selected ? tui_palette_attr(THEME_SLOT_SIDEBAR_SELECTED) | ATTR_BOLD : tui_palette_attr(THEME_SLOT_COMPOSER);
        tui_fill((UiRect){ y0 + k, anchor.x, 1, w }, attr);
        char line[200];
        snprintf(line, sizeof(line), " /%s %s", c->name, c->args);
        int used = tui_text(y0 + k, anchor.x, 28, line, attr);
        tui_text(y0 + k, anchor.x + (used > 28 ? used : 28), w - 29, c->help, attr | ATTR_DIM);
    }
}
