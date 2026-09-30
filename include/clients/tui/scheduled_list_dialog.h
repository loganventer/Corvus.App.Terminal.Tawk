#ifndef APP_CLIENTS_TUI_SCHEDULED_LIST_DIALOG_H
#define APP_CLIENTS_TUI_SCHEDULED_LIST_DIALOG_H

#include <stddef.h>

#include "clients/tui/scheduled_list_request.h"
#include "clients/tui/text_caret.h"
#include "clients/tui/text_field.h"
#include "clients/tui/ui_rect.h"
#include "core/scheduled_message.h"

#define SCHEDULED_LIST_ROWS 64

/* Every message waiting to be sent later, soonest first: when, to whom and
 * the text, with Send now, Change time and Cancel for the highlighted one.
 * Change time opens a line to type the new time in (18:00, +1h, tomorrow
 * 9:00); reading it is the owner's job. */
typedef struct ScheduledListDialog {
    int       open;
    int       selected;
    int       scroll;
    char      ids[SCHEDULED_LIST_ROWS][64];     /* the messages shown, from the last render */
    int       count;
    int       editing;                          /* typing a new time */
    TextField when;
    char      error[160];
    TextCaret caret;
    UiRect    last_rect;
    UiRect    list_rect;
    UiRect    send_button;
    UiRect    time_button;
    UiRect    cancel_button;
} ScheduledListDialog;

void                 scheduled_list_dialog_open(ScheduledListDialog *dialog);
ScheduledListRequest scheduled_list_dialog_key(ScheduledListDialog *dialog, int is_key_code, int ch);
ScheduledListRequest scheduled_list_dialog_click(ScheduledListDialog *dialog, int y, int x);
/* The highlighted message's id, or NULL when there is none. */
const char          *scheduled_list_dialog_selected(const ScheduledListDialog *dialog);
/* The new time typed, as UTF-8; the caller frees it. */
char                *scheduled_list_dialog_when(const ScheduledListDialog *dialog);
/* After RESCHEDULE: it worked (back to the list), or why not. */
void                 scheduled_list_dialog_rescheduled(ScheduledListDialog *dialog);
void                 scheduled_list_dialog_error(ScheduledListDialog *dialog, const char *why);
/* `name_of` gives a chat's name for its JID. */
void                 scheduled_list_dialog_render(ScheduledListDialog *dialog, UiRect area, const ScheduledMessage *items, int count,
                                                  void (*name_of)(void *ctx, const char *jid, char *out, size_t size), void *ctx,
                                                  int use_24h);

#endif
