#ifndef APP_CLIENTS_TUI_REACTION_PALETTE_H
#define APP_CLIENTS_TUI_REACTION_PALETTE_H

#include "clients/tui/popup_result.h"
#include "clients/tui/ui_rect.h"

#define REACTION_PALETTE_SIZE 8   /* six emoji, "remove" and "more" */

/* Quick reactions for the selected message, like the long-press bar. */
typedef struct ReactionPalette {
    int    open;
    int    selected;
    char   message_id[64];
    int    item_x[REACTION_PALETTE_SIZE + 1];
    UiRect last_rect;
} ReactionPalette;

void        reaction_palette_open(ReactionPalette *palette, const char *message_id);
PopupResult reaction_palette_key(ReactionPalette *palette, int is_key_code, int ch);
PopupResult reaction_palette_click(ReactionPalette *palette, int y, int x);
/* True when "more" was chosen: open the full emoji picker instead. */
int         reaction_palette_wants_more(const ReactionPalette *palette);
/* The chosen emoji; "" means remove our reaction. */
const char *reaction_palette_choice(const ReactionPalette *palette);
void        reaction_palette_render(ReactionPalette *palette, UiRect area);

#endif
