#ifndef APP_MANAGERS_ACCOUNT_LINK_GUARD_DEPS_H
#define APP_MANAGERS_ACCOUNT_LINK_GUARD_DEPS_H

#include "contracts/i_account_link_expectation.h"
#include "contracts/i_message_gateway.h"
#include "contracts/i_notifier.h"
#include "core/account_id.h"

/* What the link guard depends on, injected by the composition root. */
typedef struct AccountLinkGuardDeps {
    IAccountLinkExpectation *expectation;
    IMessageGateway         *gateway;
    INotifier               *notifier;
    AccountId                account;
    const char              *label;
} AccountLinkGuardDeps;

#endif
