#ifndef APP_CLIENTS_TUI_PRIVATE_UNLOCK_STATE_H
#define APP_CLIENTS_TUI_PRIVATE_UNLOCK_STATE_H

/* What the unlock dialog of a private chat is showing. */
typedef enum {
    PRIVATE_UNLOCK_WAITING_TOUCH_ID = 0,   /* the Touch ID sheet is up */
    PRIVATE_UNLOCK_PASSWORD,               /* the lock password is typed here */
    PRIVATE_UNLOCK_WAIT                    /* wrong tries: the next is taken after a wait */
} PrivateUnlockState;

#endif
