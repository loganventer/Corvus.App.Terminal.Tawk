#ifndef APP_CONTRACTS_I_MESSAGE_STORE_H
#define APP_CONTRACTS_I_MESSAGE_STORE_H

#include "core/message.h"

typedef struct IMessageStore {
    void *ctx;
    int  (*save)(struct IMessageStore *self, const Message *msg);
    /* Newest `limit` messages of a chat, returned oldest first. Caller frees with message_array_free. */
    int  (*recent)(struct IMessageStore *self, const char *jid, int limit, Message **out, int *count);
    /* Up to `limit` messages sent before `before` (epoch seconds), oldest first. Caller frees. */
    int  (*before)(struct IMessageStore *self, const char *jid, int64_t before, int limit, Message **out, int *count);
    int  (*get)(struct IMessageStore *self, const char *id, Message *out);
    int  (*update_status)(struct IMessageStore *self, const char *id, MessageStatus status);
    int  (*set_media_path)(struct IMessageStore *self, const char *id, const char *path);
    /* Replaces the text after an edit, or clears it when deleted for everyone. */
    int  (*edit_text)(struct IMessageStore *self, const char *id, const char *text, int deleted);
    /* Removes a message (deleted for me). */
    int  (*remove)(struct IMessageStore *self, const char *id);
    /* Removes every message of a chat (and their reactions). */
    int  (*remove_chat)(struct IMessageStore *self, const char *jid);
    /* Full-text search across all chats, newest first. Caller frees. */
    int  (*search)(struct IMessageStore *self, const char *query, int limit, Message **out, int *count);
    /* Moves every message of chat or sender `from` to `to` (alias merge). */
    int  (*reassign_jid)(struct IMessageStore *self, const char *from, const char *to);
    void (*destroy)(struct IMessageStore *self);
} IMessageStore;

#endif
