#ifndef APP_CLIENTS_TUI_PRIVATE_REQUEST_H
#define APP_CLIENTS_TUI_PRIVATE_REQUEST_H

#include "clients/tui/private_action.h"
#include "core/account.h"
#include "core/account_id.h"
#include "core/unlocked_chat.h"

/* One thing asked of a private chat, kept while Touch ID or the password is
 * asked for. A chat merged across accounts lists each account's side. */
typedef struct PrivateRequest {
    PrivateAction action;
    AccountId     account;                 /* the account it is seen through */
    char          jid[128];
    UnlockedChat  peers[ACCOUNT_MAX];      /* its other accounts' sides, for a merged chat */
    int           peer_count;
    int           with_media;              /* an export: the chat's media go with it */
} PrivateRequest;

#endif
