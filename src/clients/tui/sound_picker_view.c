#include "clients/tui/sound_picker_view.h"
#include "clients/tui/tui_draw.h"
#include "clients/tui/tui_palette.h"

#include <stdio.h>
#include <string.h>

void sound_picker_view_render(SoundPicker *p, ISoundCatalog *c, UiRect list, const char *current_path) {
    int n = c->count(c);
    p->list_y = list.y;
    p->list_h = list.h;
    if (n == 0) {
        tui_text(list.y, list.x, list.w, " No sound files found in the sounds folders", tui_palette_attr(THEME_SLOT_DIM));
        return;
    }
    if (p->index < p->scroll) p->scroll = p->index;
    if (p->index >= p->scroll + list.h) p->scroll = p->index - list.h + 1;
    for (int k = 0; k < list.h && p->scroll + k < n; k++) {
        int i = p->scroll + k;
        const SoundOption *option = c->at(c, i);
        int selected = i == p->index;
        int attr = selected ? tui_palette_attr(THEME_SLOT_SIDEBAR_SELECTED) | ATTR_BOLD : tui_palette_attr(THEME_SLOT_BASE);
        tui_fill((UiRect){ list.y + k, list.x, 1, list.w }, attr);
        char line[256];
        snprintf(line, sizeof(line), " %s %s",
                 current_path && strcmp(option->path, current_path) == 0 ? "\xE2\x9C\x93" : " ", option->id);
        tui_text(list.y + k, list.x, list.w, line, attr);
    }
}
