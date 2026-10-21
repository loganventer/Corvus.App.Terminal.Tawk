#ifndef APP_CLIENTS_TUI_REACTIONS_DIALOG_H
#define APP_CLIENTS_TUI_REACTIONS_DIALOG_H

#include "clients/tui/popup_result.h"
#include "clients/tui/ui_rect.h"
#include "core/reaction.h"
#include "engines/reaction_grouping.h"

/* Who reacted to a message in a group and with what, as on the phone: a
 * line a person, the emoji most used first, and the counts in the title. */
typedef struct ReactionsDialog {
    int    open;
    char   message_id[64];
    int    selected;
    int    scroll;
    int    count;                     /* people in the last render */
    UiRect last_rect;
} ReactionsDialog;

void        reactions_dialog_open(ReactionsDialog *dialog, const char *message_id);
/* POPUP_CLOSED on Esc, q or ←. */
PopupResult reactions_dialog_key(ReactionsDialog *dialog, int is_key_code, int ch);
/* POPUP_CLOSED on a click outside it. */
PopupResult reactions_dialog_click(ReactionsDialog *dialog, int y, int x);
void        reactions_dialog_wheel(ReactionsDialog *dialog, int delta);
/* `items` are drawn sorted by reaction_grouping_sort; each needs its name filled in. */
void        reactions_dialog_render(ReactionsDialog *dialog, UiRect area, const Reaction *items, int count);

#endif
