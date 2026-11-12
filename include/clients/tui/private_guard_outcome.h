#ifndef APP_CLIENTS_TUI_PRIVATE_GUARD_OUTCOME_H
#define APP_CLIENTS_TUI_PRIVATE_GUARD_OUTCOME_H

/* Where a request to the private chat guard stands. */
typedef enum {
    PRIVATE_GUARD_WAITING = 0,   /* still asking, or nothing asked */
    PRIVATE_GUARD_DONE,          /* passed: carry the request out now */
    PRIVATE_GUARD_CANCELLED      /* given up: nothing is done */
} PrivateGuardOutcome;

#endif
