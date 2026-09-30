#ifndef APP_CLIENTS_TUI_CONFIRM_DIALOG_H
#define APP_CLIENTS_TUI_CONFIRM_DIALOG_H

#include <stdint.h>

#include "clients/tui/confirm_purpose.h"
#include "clients/tui/popup_result.h"
#include "clients/tui/ui_rect.h"

/* A yes-or-no question in the middle of the screen. Cancel is selected
 * first. A dangerous one (permanent deletion) flashes its border and
 * warning so it cannot be missed. The dialog knows nothing about what it
 * confirms: the purpose and subject go back to the caller. */
typedef struct ConfirmDialog {
    int            open;
    ConfirmPurpose purpose;
    char           subject[128];      /* what it is about (a chat's JID) */
    char           title[96];
    char           question[160];
    char           warning[256];
    char           confirm_label[48];
    int            danger;
    int            confirm_selected;  /* 0 Cancel, 1 the action */
    UiRect         last_rect;
    UiRect         cancel_button;
    UiRect         confirm_button;
} ConfirmDialog;

void        confirm_dialog_open(ConfirmDialog *dialog, ConfirmPurpose purpose, const char *subject,
                                const char *title, const char *question, const char *warning,
                                const char *confirm_label, int danger);
/* POPUP_CHOSEN when confirmed, POPUP_CLOSED when cancelled. */
PopupResult confirm_dialog_key(ConfirmDialog *dialog, int is_key_code, int ch);
PopupResult confirm_dialog_click(ConfirmDialog *dialog, int y, int x);
void        confirm_dialog_render(ConfirmDialog *dialog, UiRect area, int64_t now_ms);

#endif
