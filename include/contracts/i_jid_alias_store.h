#ifndef APP_CONTRACTS_I_JID_ALIAS_STORE_H
#define APP_CONTRACTS_I_JID_ALIAS_STORE_H

#include <stddef.h>

/* Remembers that one JID is another name for a canonical JID (a LID for a
 * phone number), so the same person never appears as two chats. */
typedef struct IJidAliasStore {
    void *ctx;
    int         (*put)(struct IJidAliasStore *self, const char *alias, const char *canonical);
    /* The canonical JID for `jid`, or `jid` itself when it has no alias. */
    const char *(*resolve)(struct IJidAliasStore *self, const char *jid);
    void        (*destroy)(struct IJidAliasStore *self);
} IJidAliasStore;

#endif
