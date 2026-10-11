#ifndef APP_CONTRACTS_I_ACCOUNT_LINK_EXPECTATION_H
#define APP_CONTRACTS_I_ACCOUNT_LINK_EXPECTATION_H

#include <stddef.h>

#include "core/account_id.h"

/* The number a moved account must be linked as, so its chats are never
 * attached to a different person's WhatsApp. */
typedef struct IAccountLinkExpectation {
    void *ctx;
    int  (*expect)(struct IAccountLinkExpectation *self, AccountId id, const char *jid);
    /* "" when none is expected. */
    int  (*expected)(struct IAccountLinkExpectation *self, AccountId id, char *out, size_t size);
    int  (*clear)(struct IAccountLinkExpectation *self, AccountId id);
    void (*destroy)(struct IAccountLinkExpectation *self);
} IAccountLinkExpectation;

#endif
