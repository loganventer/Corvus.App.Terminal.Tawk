#ifndef APP_MANAGERS_ACCOUNT_LINK_GUARD_H
#define APP_MANAGERS_ACCOUNT_LINK_GUARD_H

#include "contracts/i_event_observer.h"
#include "managers/account_link_guard_deps.h"

/* Watches one account's link events. An account that was moved here from
 * another computer expects the number it came from: when that number links
 * the expectation is cleared, and when another number does, tawk unlinks it
 * again and says why, so the moved chats are never attached to someone
 * else's WhatsApp. An account that was not moved is not touched. */
typedef struct AccountLinkGuard AccountLinkGuard;

AccountLinkGuard *account_link_guard_create(const AccountLinkGuardDeps *deps);
void              account_link_guard_destroy(AccountLinkGuard *guard);
IEventObserver   *account_link_guard_observer(AccountLinkGuard *guard);

#endif
