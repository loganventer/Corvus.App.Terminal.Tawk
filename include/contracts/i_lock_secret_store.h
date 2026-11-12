#ifndef APP_CONTRACTS_I_LOCK_SECRET_STORE_H
#define APP_CONTRACTS_I_LOCK_SECRET_STORE_H

#include "core/lock_verifier.h"

/* Where the lock password for private chats is kept, as its verifier. There
 * is one for the whole computer, or none until it is set. */
typedef struct ILockSecretStore {
    void *ctx;
    /* 1 and fills `out` when there is one, 0 when there is none, -1 on error. */
    int  (*load)(struct ILockSecretStore *self, LockVerifier *out);
    /* Replaces whatever was there. */
    int  (*save)(struct ILockSecretStore *self, const LockVerifier *verifier);
    int  (*clear)(struct ILockSecretStore *self);
    void (*destroy)(struct ILockSecretStore *self);
} ILockSecretStore;

#endif
