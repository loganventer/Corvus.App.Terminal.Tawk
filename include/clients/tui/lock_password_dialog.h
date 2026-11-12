#ifndef APP_CLIENTS_TUI_LOCK_PASSWORD_DIALOG_H
#define APP_CLIENTS_TUI_LOCK_PASSWORD_DIALOG_H

#include "clients/tui/lock_password_key.h"
#include "clients/tui/masked_field.h"
#include "clients/tui/ui_rect.h"
#include "core/lock_password_problem.h"
#include "core/passphrase.h"

/* Where a new lock password for private chats is typed twice: the first
 * time one is set, and through /lockpassword. Tab moves between the two
 * fields; Enter on the first moves on, on the second hands both over. */
typedef struct LockPasswordDialog {
    int                 open;
    int                 first;       /* no lock password yet: this sets the first */
    int                 focus;       /* 0 the new password, 1 the same again */
    MaskedField         fields[2];
    LockPasswordProblem problem;     /* what was wrong with the last try */
    UiRect              last_rect;
} LockPasswordDialog;

void            lock_password_dialog_open(LockPasswordDialog *dialog, int first);
LockPasswordKey lock_password_dialog_key(LockPasswordDialog *dialog, int is_key, int ch);
/* Hands both over and clears the fields. */
void            lock_password_dialog_take(LockPasswordDialog *dialog, Passphrase *password, Passphrase *confirm);
/* The last try was refused: says why and starts again at the first field. */
void            lock_password_dialog_refused(LockPasswordDialog *dialog, LockPasswordProblem problem);
/* Closes it and wipes both fields. */
void            lock_password_dialog_close(LockPasswordDialog *dialog);
void            lock_password_dialog_render(LockPasswordDialog *dialog, UiRect area);

#endif
