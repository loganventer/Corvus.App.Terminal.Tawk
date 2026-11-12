#include "engines/private_chat_session.h"
#include "utilities/secure_zero.h"

#include <stdio.h>
#include <string.h>

static void put(UnlockedChat *slot, AccountId account, const char *jid) {
    slot->account = account;
    snprintf(slot->jid, sizeof(slot->jid), "%s", jid ? jid : "");
}

void private_chat_session_relock_all(PrivateChatSession *session) {
    secure_zero(session, sizeof(*session));
}

void private_chat_session_unlock(PrivateChatSession *session, AccountId account, const char *jid) {
    private_chat_session_relock_all(session);
    if (!jid || !jid[0]) return;
    put(&session->chats[0], account, jid);
    session->count = 1;
}

void private_chat_session_add_peer(PrivateChatSession *session, AccountId account, const char *jid) {
    if (session->count == 0 || !jid || !jid[0]) return;
    if (private_chat_session_is_unlocked(session, account, jid)) return;
    if (session->count >= ACCOUNT_MAX) return;
    put(&session->chats[session->count++], account, jid);
}

int private_chat_session_is_unlocked(const PrivateChatSession *session, AccountId account, const char *jid) {
    if (!jid) return 0;
    for (int i = 0; i < session->count; i++)
        if (session->chats[i].account == account && strcmp(session->chats[i].jid, jid) == 0) return 1;
    return 0;
}

void private_chat_session_leave(PrivateChatSession *session, AccountId account, const char *jid) {
    if (!private_chat_session_is_unlocked(session, account, jid)) private_chat_session_relock_all(session);
}
