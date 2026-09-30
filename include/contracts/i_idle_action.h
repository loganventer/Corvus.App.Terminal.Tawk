#ifndef APP_CONTRACTS_I_IDLE_ACTION_H
#define APP_CONTRACTS_I_IDLE_ACTION_H

/* Polled while the idle action runs; return non-zero to stop it early. */
typedef int (*IdleWakeCheck)(void *user);

typedef struct IIdleAction {
    void *ctx;
    /* Runs `command` full screen until a key is pressed, the command exits,
     * or wake_check returns non-zero. Blocks the caller. */
    int  (*run)(struct IIdleAction *self, const char *command, IdleWakeCheck wake_check, void *user);
    void (*destroy)(struct IIdleAction *self);
} IIdleAction;

#endif
