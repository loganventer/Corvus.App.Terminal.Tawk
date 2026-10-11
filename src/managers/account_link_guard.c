#include "managers/account_link_guard.h"
#include "engines/account_identity_rule.h"
#include "utilities/log.h"
#include "utilities/str_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct AccountLinkGuard {
    AccountLinkGuardDeps deps;
    IEventObserver       observer;
};

static int on_event(IEventObserver *self, const Event *e) {
    AccountLinkGuard *g = self->ctx;
    if (e->type != EVENT_AUTH_CONNECTED) return 0;
    char expected[128];
    if (g->deps.expectation->expected(g->deps.expectation, g->deps.account, expected, sizeof(expected)) != 0) return 0;
    IdentityVerdict verdict = account_identity_rule(expected, e->jid);
    if (verdict == IDENTITY_NOT_EXPECTED) return 0;
    if (verdict == IDENTITY_MATCH) {
        g->deps.expectation->clear(g->deps.expectation, g->deps.account);
        return 1;
    }
    LOG_WARN("account %d was moved from another number than the one that linked; unlinking it again", g->deps.account);
    if (g->deps.gateway && g->deps.gateway->logout) g->deps.gateway->logout(g->deps.gateway);
    if (g->deps.notifier) {
        Notification n;
        memset(&n, 0, sizeof(n));
        str_copy(n.title, sizeof(n.title), "Wrong number for a moved account");
        str_copy(n.body, sizeof(n.body),
                 "That is not the number this account was moved from, so it was unlinked again. "
                 "Scan the QR code with the same phone, or remove the account.");
        n.account = g->deps.account;
        if (g->deps.label) str_copy(n.account_label, sizeof(n.account_label), g->deps.label);
        g->deps.notifier->notify(g->deps.notifier, &n);
    }
    return 1;
}

static void observer_destroy(IEventObserver *self) { (void)self; }   /* owned by the guard */

AccountLinkGuard *account_link_guard_create(const AccountLinkGuardDeps *deps) {
    AccountLinkGuard *g = calloc(1, sizeof(*g));
    if (!g) return NULL;
    g->deps = *deps;
    g->observer.ctx = g;
    g->observer.on_event = on_event;
    g->observer.destroy = observer_destroy;
    return g;
}

void account_link_guard_destroy(AccountLinkGuard *g) { free(g); }

IEventObserver *account_link_guard_observer(AccountLinkGuard *g) { return &g->observer; }
