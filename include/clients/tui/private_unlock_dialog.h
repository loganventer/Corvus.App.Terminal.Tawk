#ifndef APP_CLIENTS_TUI_PRIVATE_UNLOCK_DIALOG_H
#define APP_CLIENTS_TUI_PRIVATE_UNLOCK_DIALOG_H

#include <stdint.h>

#include "clients/tui/masked_field.h"
#include "clients/tui/private_unlock_key.h"
#include "clients/tui/private_unlock_state.h"
#include "clients/tui/ui_rect.h"
#include "core/passphrase.h"
#include "core/unlock_purpose.h"

/* The box in the middle of the screen while a private chat is asked for:
 * Touch ID, the lock password, or the wait after wrong tries. It says what
 * the unlock is for and never shows the chat or its messages. It only
 * draws and takes keys; checking is someone else's work. */
typedef struct PrivateUnlockDialog {
    int                open;
    PrivateUnlockState state;
    UnlockPurpose      purpose;
    MaskedField        field;
    int                wrong;          /* the last password typed was wrong */
    int64_t            retry_at_ms;    /* in WAIT: when the next try is taken */
    UiRect             last_rect;
} PrivateUnlockDialog;

void             private_unlock_dialog_open(PrivateUnlockDialog *dialog, UnlockPurpose purpose, int touch_id);
/* Moves from Touch ID to the password field. */
void             private_unlock_dialog_show_password(PrivateUnlockDialog *dialog);
/* The password was wrong; with wait_ms > 0 the next try waits that long. */
void             private_unlock_dialog_wrong(PrivateUnlockDialog *dialog, int64_t wait_ms, int64_t now_ms);
PrivateUnlockKey private_unlock_dialog_key(PrivateUnlockDialog *dialog, int is_key, int ch, int64_t now_ms);
/* Hands over what was typed and clears the field. */
void             private_unlock_dialog_take(PrivateUnlockDialog *dialog, Passphrase *out);
/* Closes it and wipes what was typed. */
void             private_unlock_dialog_close(PrivateUnlockDialog *dialog);
void             private_unlock_dialog_render(PrivateUnlockDialog *dialog, UiRect area, int64_t now_ms);

#endif
