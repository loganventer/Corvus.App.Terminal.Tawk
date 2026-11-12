#ifndef APP_ENGINES_PRIVATE_CHAT_POLICY_H
#define APP_ENGINES_PRIVATE_CHAT_POLICY_H

#include "core/account_id.h"
#include "core/chat.h"
#include "core/unlock_purpose.h"
#include "engines/private_chat_session.h"

/* The one place that says what a private chat shows and when it asks first.
 * `account` is the account the chat is seen through: the row's own account
 * where several are listed together, otherwise the account in view. */

/* Non-zero when the chat must be drawn veiled: private and not unlocked. */
int private_chat_policy_veiled(const Chat *chat, AccountId account, const PrivateChatSession *session);
/* Non-zero when `purpose` needs Touch ID or the lock password first. Opening
 * asks only while the chat is locked; making it public and exporting it ask
 * every time; changing the password always asks. A public chat never asks. */
int private_chat_policy_needs_unlock(UnlockPurpose purpose, const Chat *chat, AccountId account, const PrivateChatSession *session);
/* Non-zero when tawk's own search may list the chat's messages. */
int private_chat_policy_tui_searchable(const Chat *chat, AccountId account, const PrivateChatSession *session);

#endif
