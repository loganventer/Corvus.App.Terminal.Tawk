#ifndef APP_CONTRACTS_I_PASSPHRASE_PROMPT_H
#define APP_CONTRACTS_I_PASSPHRASE_PROMPT_H

#include "core/passphrase.h"

/* Asks the person at the terminal for a passphrase without showing it. */
typedef struct IPassphrasePrompt {
    void *ctx;
    /* Shows `prompt` and reads the answer into `out`. With `confirm`, asks
     * again and only accepts it when both match. Returns 0 on success, -1
     * when nothing usable was typed (empty, too long, a mismatch or no terminal). */
    int  (*ask)(struct IPassphrasePrompt *self, const char *prompt, int confirm, Passphrase *out);
    void (*destroy)(struct IPassphrasePrompt *self);
} IPassphrasePrompt;

#endif
