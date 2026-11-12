#ifndef APP_ENGINES_PRIVATE_CHAT_SESSION_H
#define APP_ENGINES_PRIVATE_CHAT_SESSION_H

#include "core/account.h"
#include "core/account_id.h"
#include "core/unlocked_chat.h"

/* The one private chat that is unlocked, if any. A chat merged across
 * accounts is one conversation, so each account's side of it is unlocked
 * with it. Opening any other chat locks it again. It keeps state but does
 * no I/O. */
typedef struct PrivateChatSession {
    UnlockedChat chats[ACCOUNT_MAX];   /* the unlocked chat first, then its merged peers */
    int          count;                /* 0 when nothing is unlocked */
} PrivateChatSession;

/* Unlocks this chat alone; whatever was unlocked before is locked. */
void private_chat_session_unlock(PrivateChatSession *session, AccountId account, const char *jid);
/* Unlocks another account's side of the chat that is unlocked. Does
 * nothing when nothing is unlocked. */
void private_chat_session_add_peer(PrivateChatSession *session, AccountId account, const char *jid);
int  private_chat_session_is_unlocked(const PrivateChatSession *session, AccountId account, const char *jid);
/* Call with the chat that is opening: unless it is the unlocked chat or one
 * of its peers, everything is locked again. */
void private_chat_session_leave(PrivateChatSession *session, AccountId account, const char *jid);
void private_chat_session_relock_all(PrivateChatSession *session);

#endif
