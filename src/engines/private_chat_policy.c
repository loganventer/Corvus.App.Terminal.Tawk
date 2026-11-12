#include "engines/private_chat_policy.h"

static int unlocked(const Chat *chat, AccountId account, const PrivateChatSession *session) {
    return session && private_chat_session_is_unlocked(session, account, chat->jid);
}

int private_chat_policy_veiled(const Chat *chat, AccountId account, const PrivateChatSession *session) {
    return chat && chat->is_private && !unlocked(chat, account, session);
}

int private_chat_policy_needs_unlock(UnlockPurpose purpose, const Chat *chat, AccountId account, const PrivateChatSession *session) {
    if (purpose == UNLOCK_FOR_PASSWORD_CHANGE) return 1;
    if (!chat || !chat->is_private) return 0;
    switch (purpose) {
    case UNLOCK_FOR_OPEN:        return !unlocked(chat, account, session);
    case UNLOCK_FOR_MAKE_PUBLIC:
    case UNLOCK_FOR_EXPORT:      return 1;
    default:                     return 1;
    }
}

int private_chat_policy_tui_searchable(const Chat *chat, AccountId account, const PrivateChatSession *session) {
    return chat && !private_chat_policy_veiled(chat, account, session);
}
