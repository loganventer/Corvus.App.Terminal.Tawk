#ifndef APP_CONTRACTS_I_SCHEDULED_MESSAGE_STORE_H
#define APP_CONTRACTS_I_SCHEDULED_MESSAGE_STORE_H

#include <stdint.h>

#include "core/scheduled_message.h"

/* Messages to send later, kept until they go. Lists come back as an array
 * the caller frees with scheduled_message_array_free. */
typedef struct IScheduledMessageStore {
    void *ctx;
    int  (*add)(struct IScheduledMessageStore *self, const ScheduledMessage *message);
    int  (*get)(struct IScheduledMessageStore *self, const char *id, ScheduledMessage *out);
    int  (*set_due)(struct IScheduledMessageStore *self, const char *id, int64_t due_at);
    int  (*set_state)(struct IScheduledMessageStore *self, const char *id, ScheduledState state);
    int  (*remove)(struct IScheduledMessageStore *self, const char *id);
    /* Waiting messages, soonest first: for one chat, or every chat when `chat_jid` is NULL. */
    int  (*list_waiting)(struct IScheduledMessageStore *self, const char *chat_jid, ScheduledMessage **out, int *count);
    /* Waiting messages due at or before `now`, soonest first. */
    int  (*due)(struct IScheduledMessageStore *self, int64_t now, ScheduledMessage **out, int *count);
    /* A chat's hidden id (LID) turned out to be this phone number. */
    int  (*reassign_jid)(struct IScheduledMessageStore *self, const char *from, const char *to);
    void (*destroy)(struct IScheduledMessageStore *self);
} IScheduledMessageStore;

#endif
