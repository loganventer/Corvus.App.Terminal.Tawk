#ifndef APP_CONTRACTS_I_FRAME_HOOK_H
#define APP_CONTRACTS_I_FRAME_HOOK_H

/* Work another client does on the UI thread once a frame, so the terminal
 * client can run it without knowing what it is. Returns 1 when something
 * it did may change what is shown. */
typedef struct IFrameHook {
    void *ctx;
    int  (*tick)(struct IFrameHook *self);
} IFrameHook;

#endif
