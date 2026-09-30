#ifndef APP_CLIENTS_TUI_STATUS_VIEWERS_DIALOG_H
#define APP_CLIENTS_TUI_STATUS_VIEWERS_DIALOG_H

#include <stddef.h>

#include "clients/tui/popup_result.h"
#include "clients/tui/ui_rect.h"
#include "core/status_viewer.h"

#define STATUS_VIEWERS_ROWS 64

/* Who saw one of your statuses, as on the phone: newest first, with the
 * time each saw it and a heart beside those who liked it. */
typedef struct StatusViewersDialog {
    int    open;
    int    selected;
    int    scroll;
    int    count;                     /* viewers in the last render */
    UiRect last_rect;
} StatusViewersDialog;

void        status_viewers_dialog_open(StatusViewersDialog *dialog);
/* POPUP_CLOSED back to the status (Esc, q, ← or a click outside). */
PopupResult status_viewers_dialog_key(StatusViewersDialog *dialog, int is_key_code, int ch);
PopupResult status_viewers_dialog_click(StatusViewersDialog *dialog, int y, int x);
void        status_viewers_dialog_wheel(StatusViewersDialog *dialog, int delta);
/* `name_of` gives the name shown for a viewer's JID. */
void        status_viewers_dialog_render(StatusViewersDialog *dialog, UiRect area, const StatusViewer *viewers, int count,
                                         void (*name_of)(void *ctx, const char *jid, char *out, size_t size), void *ctx,
                                         int use_24h);

#endif
