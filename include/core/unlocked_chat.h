#ifndef APP_CORE_UNLOCKED_CHAT_H
#define APP_CORE_UNLOCKED_CHAT_H

#include "core/account_id.h"

/* One account's side of a private chat that is open right now. */
typedef struct UnlockedChat {
    AccountId account;
    char      jid[128];
} UnlockedChat;

#endif
