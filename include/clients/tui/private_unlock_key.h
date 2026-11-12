#ifndef APP_CLIENTS_TUI_PRIVATE_UNLOCK_KEY_H
#define APP_CLIENTS_TUI_PRIVATE_UNLOCK_KEY_H

/* What a key did in the unlock dialog. */
typedef enum {
    PRIVATE_UNLOCK_KEY_NONE = 0,       /* taken, nothing to do */
    PRIVATE_UNLOCK_KEY_CHANGED,        /* draw again */
    PRIVATE_UNLOCK_KEY_SUBMIT,         /* Enter: check the password */
    PRIVATE_UNLOCK_KEY_USE_PASSWORD,   /* Tab while Touch ID is up: type the password instead */
    PRIVATE_UNLOCK_KEY_CANCEL          /* Esc */
} PrivateUnlockKey;

#endif
