#ifndef APP_CLIENTS_TUI_PRIVATE_CHAT_GUARD_H
#define APP_CLIENTS_TUI_PRIVATE_CHAT_GUARD_H

#include <stdint.h>

#include "clients/tui/lock_password_dialog.h"
#include "clients/tui/private_guard_outcome.h"
#include "clients/tui/private_request.h"
#include "clients/tui/private_unlock_dialog.h"
#include "engines/private_chat_session.h"
#include "managers/privacy_manager.h"

/* Stands between the screen and private chats: asks Touch ID or the lock
 * password, holds what was asked for until it passes, and keeps which
 * chat is unlocked. A request is carried out only when the guard answers
 * PRIVATE_GUARD_DONE; nothing else lets it through. While a dialog is up
 * it takes every key. */
typedef struct PrivateChatGuard {
    PrivacyManager     *privacy;
    PrivateChatSession  session;
    PrivateUnlockDialog unlock;
    LockPasswordDialog  password;
    PrivateRequest      pending;
    int                 has_pending;
    int                 asking_biometric;   /* the Touch ID sheet is up for `pending` */
} PrivateChatGuard;

void                private_chat_guard_init(PrivateChatGuard *guard, PrivacyManager *privacy);
/* A dialog is up and takes the keys. */
int                 private_chat_guard_busy(const PrivateChatGuard *guard);
/* Asks for what `request` needs. DONE when it may be carried out at once
 * (making a chat private with a password already set); otherwise WAITING
 * while a dialog is up. Opening, exporting and making public are asked
 * only when the caller found they need it. */
PrivateGuardOutcome private_chat_guard_request(PrivateChatGuard *guard, const PrivateRequest *request, int64_t now_ms);
/* Once a frame: follows the Touch ID sheet. On DONE `*done` is the request to carry out. */
PrivateGuardOutcome private_chat_guard_tick(PrivateChatGuard *guard, int64_t now_ms, PrivateRequest *done);
/* A key while busy. On DONE `*done` is the request to carry out. */
PrivateGuardOutcome private_chat_guard_key(PrivateChatGuard *guard, int is_key, int ch, int64_t now_ms, PrivateRequest *done);
/* Another chat is opening: the unlocked one locks again unless it is that one. */
void                private_chat_guard_leave(PrivateChatGuard *guard, AccountId account, const char *jid);
/* Everything locks again, and anything being asked is dropped. */
void                private_chat_guard_relock_all(PrivateChatGuard *guard);

#endif
