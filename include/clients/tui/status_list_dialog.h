#ifndef APP_CLIENTS_TUI_STATUS_LIST_DIALOG_H
#define APP_CLIENTS_TUI_STATUS_LIST_DIALOG_H

#include "clients/tui/popup_result.h"
#include "clients/tui/ui_rect.h"
#include "core/status_author.h"

#define STATUS_LIST_ROWS 64

/* The Updates list, like WhatsApp's Status tab: My status first, then the
 * people with new statuses, then those already seen. Tab switches to the
 * archive of statuses older than a day that tawk has kept. */
typedef struct StatusListDialog {
    int    open;
    int    archived;                    /* showing statuses older than a day (Tab switches) */
    int    selected;                    /* position in the list as shown */
    int    scroll;
    int    count;                       /* authors in the last render */
    int    order[STATUS_LIST_ROWS];     /* position shown -> index in the authors given */
    UiRect last_rect;
    UiRect rows[STATUS_LIST_ROWS];      /* per drawn row */
    int    row_position[STATUS_LIST_ROWS];
    int    row_count;
    UiRect post_button;
    UiRect tabs[2];                     /* Recent, Archive */
} StatusListDialog;

void        status_list_dialog_open(StatusListDialog *dialog);
/* POPUP_CHOSEN: view the author from _choice. 'n', + or the New status
 * button set *post (with POPUP_CHANGED) to ask for a new status. */
PopupResult status_list_dialog_key(StatusListDialog *dialog, int is_key_code, int ch, int *post);
PopupResult status_list_dialog_click(StatusListDialog *dialog, int y, int x, int *post);
void        status_list_dialog_wheel(StatusListDialog *dialog, int delta);
/* The chosen author's index in the array last rendered. */
int         status_list_dialog_choice(const StatusListDialog *dialog);
/* `name_of` gives the name to show for an author (NULL uses its push name). */
void        status_list_dialog_render(StatusListDialog *dialog, UiRect area, const StatusAuthor *authors, int count,
                                      const char *(*name_of)(void *ctx, const StatusAuthor *author), void *ctx, int use_24h);

#endif
