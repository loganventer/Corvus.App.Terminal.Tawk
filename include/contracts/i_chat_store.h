#ifndef APP_CONTRACTS_I_CHAT_STORE_H
#define APP_CONTRACTS_I_CHAT_STORE_H

#include <stdint.h>

#include "core/chat.h"

typedef struct IChatStore {
    void *ctx;
    /* Merges: empty names and older timestamps never overwrite; local
     * mute and pin preferences are preserved. */
    int  (*upsert)(struct IChatStore *self, const Chat *chat);
    /* Records a newer last message, creating the chat when missing. */
    int  (*touch)(struct IChatStore *self, const char *jid, int64_t ts, const char *preview);
    int  (*get_all)(struct IChatStore *self, Chat **out, int *count);
    int  (*get)(struct IChatStore *self, const char *jid, Chat *out);
    int  (*set_unread)(struct IChatStore *self, const char *jid, int unread);
    int  (*add_unread)(struct IChatStore *self, const char *jid, int delta);
    /* An unread message mentions you; cleared with the unread count. */
    int  (*mark_mention)(struct IChatStore *self, const char *jid);
    int  (*set_muted)(struct IChatStore *self, const char *jid, int muted);
    int  (*set_pinned)(struct IChatStore *self, const char *jid, int pinned);
    /* 0 unmutes, -1 mutes until unmuted, otherwise until that epoch second. */
    int  (*set_muted_until)(struct IChatStore *self, const char *jid, int64_t until);
    int  (*set_tone)(struct IChatStore *self, const char *jid, const char *tone);
    /* Conversation theme id for this chat; "" uses the app theme. */
    int  (*set_theme)(struct IChatStore *self, const char *jid, const char *theme);
    int  (*set_archived)(struct IChatStore *self, const char *jid, int archived);
    /* Soft lock: the conversation is blurred until shown again (kept across restarts). */
    int  (*set_soft_locked)(struct IChatStore *self, const char *jid, int locked);
    /* Forgets a chat and its preferences. */
    int  (*remove)(struct IChatStore *self, const char *jid);
    int  (*set_draft)(struct IChatStore *self, const char *jid, const char *draft);
    /* malloc'd draft text ("" when none); caller frees. */
    char *(*get_draft)(struct IChatStore *self, const char *jid);
    /* Folds chat `from` into `to` (names, unread, pins) and removes `from`. */
    int  (*merge)(struct IChatStore *self, const char *from, const char *to);
    void (*destroy)(struct IChatStore *self);
} IChatStore;

#endif
