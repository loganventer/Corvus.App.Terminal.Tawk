#ifndef APP_CLIENTS_TUI_LOCK_PASSWORD_KEY_H
#define APP_CLIENTS_TUI_LOCK_PASSWORD_KEY_H

/* What a key did in the lock password dialog. */
typedef enum {
    LOCK_PASSWORD_KEY_NONE = 0,
    LOCK_PASSWORD_KEY_CHANGED,     /* draw again */
    LOCK_PASSWORD_KEY_SUBMIT,      /* Enter on the second field: set it */
    LOCK_PASSWORD_KEY_CANCEL       /* Esc */
} LockPasswordKey;

#endif
