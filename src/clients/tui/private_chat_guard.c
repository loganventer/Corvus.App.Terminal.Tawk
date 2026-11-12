#include "clients/tui/private_chat_guard.h"
#include "utilities/secure_zero.h"

#include <string.h>

static UnlockPurpose purpose_of(PrivateAction action) {
    switch (action) {
    case PRIVATE_ACTION_OPEN:            return UNLOCK_FOR_OPEN;
    case PRIVATE_ACTION_MAKE_PUBLIC:     return UNLOCK_FOR_MAKE_PUBLIC;
    case PRIVATE_ACTION_EXPORT:          return UNLOCK_FOR_EXPORT;
    case PRIVATE_ACTION_CHANGE_PASSWORD: return UNLOCK_FOR_PASSWORD_CHANGE;
    case PRIVATE_ACTION_MAKE_PRIVATE:    break;
    }
    return UNLOCK_FOR_OPEN;
}

void private_chat_guard_init(PrivateChatGuard *g, PrivacyManager *privacy) {
    secure_zero(g, sizeof(*g));
    g->privacy = privacy;
}

int private_chat_guard_busy(const PrivateChatGuard *g) {
    return g->unlock.open || g->password.open;
}

static void drop(PrivateChatGuard *g) {
    if (g->asking_biometric) privacy_manager_cancel_biometric(g->privacy);
    g->asking_biometric = 0;
    private_unlock_dialog_close(&g->unlock);
    lock_password_dialog_close(&g->password);
    memset(&g->pending, 0, sizeof(g->pending));
    g->has_pending = 0;
}

/* The request has passed: an opened chat is unlocked with every side of it. */
static PrivateGuardOutcome pass(PrivateChatGuard *g, PrivateRequest *done) {
    PrivateRequest r = g->pending;
    drop(g);
    if (r.action == PRIVATE_ACTION_OPEN) {
        private_chat_session_unlock(&g->session, r.account, r.jid);
        for (int i = 0; i < r.peer_count; i++) private_chat_session_add_peer(&g->session, r.peers[i].account, r.peers[i].jid);
    }
    if (done) *done = r;
    return PRIVATE_GUARD_DONE;
}

static PrivateGuardOutcome cancel(PrivateChatGuard *g) {
    drop(g);
    return PRIVATE_GUARD_CANCELLED;
}

/* Touch ID did not pass but may be got round: the password, or setting the first one. */
static void ask_password(PrivateChatGuard *g) {
    g->asking_biometric = 0;
    if (privacy_manager_has_password(g->privacy)) {
        if (!g->unlock.open) private_unlock_dialog_open(&g->unlock, purpose_of(g->pending.action), 0);
        private_unlock_dialog_show_password(&g->unlock);
    } else {
        private_unlock_dialog_close(&g->unlock);
        lock_password_dialog_open(&g->password, 1);     /* no lock password on this computer yet */
    }
}

PrivateGuardOutcome private_chat_guard_request(PrivateChatGuard *g, const PrivateRequest *request, int64_t now_ms) {
    (void)now_ms;
    drop(g);
    g->pending = *request;
    g->has_pending = 1;
    int has_password = privacy_manager_has_password(g->privacy);
    if (request->action == PRIVATE_ACTION_MAKE_PRIVATE) {
        if (has_password) return pass(g, NULL);
        lock_password_dialog_open(&g->password, 1);
        return PRIVATE_GUARD_WAITING;
    }
    UnlockPurpose purpose = purpose_of(request->action);
    if (privacy_manager_biometrics_available(g->privacy) && privacy_manager_begin_biometric(g->privacy, purpose) == 0) {
        g->asking_biometric = 1;
        private_unlock_dialog_open(&g->unlock, purpose, 1);
        return PRIVATE_GUARD_WAITING;
    }
    ask_password(g);
    return PRIVATE_GUARD_WAITING;
}

/* Touch ID or the current password passed. A password change goes on to the new one. */
static PrivateGuardOutcome unlocked(PrivateChatGuard *g, PrivateRequest *done) {
    if (g->pending.action == PRIVATE_ACTION_CHANGE_PASSWORD) {
        g->asking_biometric = 0;
        private_unlock_dialog_close(&g->unlock);
        lock_password_dialog_open(&g->password, 0);
        return PRIVATE_GUARD_WAITING;
    }
    return pass(g, done);
}

PrivateGuardOutcome private_chat_guard_tick(PrivateChatGuard *g, int64_t now_ms, PrivateRequest *done) {
    (void)now_ms;
    if (!g->has_pending || !g->asking_biometric) return PRIVATE_GUARD_WAITING;
    switch (privacy_manager_poll_biometric(g->privacy)) {
    case BIOMETRIC_PENDING:     return PRIVATE_GUARD_WAITING;
    case BIOMETRIC_OK:          g->asking_biometric = 0; return unlocked(g, done);
    case BIOMETRIC_CANCELLED:   return cancel(g);
    case BIOMETRIC_FALLBACK:
    case BIOMETRIC_FAILED:
    case BIOMETRIC_UNAVAILABLE: ask_password(g); return PRIVATE_GUARD_WAITING;
    }
    return PRIVATE_GUARD_WAITING;
}

static PrivateGuardOutcome password_key(PrivateChatGuard *g, int is_key, int ch, PrivateRequest *done) {
    switch (lock_password_dialog_key(&g->password, is_key, ch)) {
    case LOCK_PASSWORD_KEY_CANCEL: return cancel(g);
    case LOCK_PASSWORD_KEY_SUBMIT: {
        Passphrase a, b;
        lock_password_dialog_take(&g->password, &a, &b);
        LockPasswordProblem problem = privacy_manager_set_password(g->privacy, &a, &b);
        if (problem != LOCK_PASSWORD_OK) { lock_password_dialog_refused(&g->password, problem); return PRIVATE_GUARD_WAITING; }
        return pass(g, done);              /* a new password: what waited for it goes ahead */
    }
    default: return PRIVATE_GUARD_WAITING;
    }
}

PrivateGuardOutcome private_chat_guard_key(PrivateChatGuard *g, int is_key, int ch, int64_t now_ms, PrivateRequest *done) {
    if (g->password.open) return password_key(g, is_key, ch, done);
    if (!g->unlock.open) return PRIVATE_GUARD_WAITING;
    switch (private_unlock_dialog_key(&g->unlock, is_key, ch, now_ms)) {
    case PRIVATE_UNLOCK_KEY_CANCEL:       return cancel(g);
    case PRIVATE_UNLOCK_KEY_USE_PASSWORD:
        privacy_manager_cancel_biometric(g->privacy);
        ask_password(g);
        return PRIVATE_GUARD_WAITING;
    case PRIVATE_UNLOCK_KEY_SUBMIT: {
        Passphrase typed;
        private_unlock_dialog_take(&g->unlock, &typed);
        int64_t wait = 0;
        PrivacyCheck check = privacy_manager_check_password(g->privacy, &typed, now_ms, &wait);
        if (check == PRIVACY_CHECK_OK) return unlocked(g, done);
        if (check == PRIVACY_CHECK_NO_PASSWORD) { ask_password(g); return PRIVATE_GUARD_WAITING; }
        private_unlock_dialog_wrong(&g->unlock, wait, now_ms);
        return PRIVATE_GUARD_WAITING;
    }
    default: return PRIVATE_GUARD_WAITING;
    }
}

void private_chat_guard_leave(PrivateChatGuard *g, AccountId account, const char *jid) {
    private_chat_session_leave(&g->session, account, jid);
}

void private_chat_guard_relock_all(PrivateChatGuard *g) {
    drop(g);
    private_chat_session_relock_all(&g->session);
}
