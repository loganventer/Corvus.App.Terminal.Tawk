#ifndef APP_CORE_SCHEDULED_MESSAGE_H
#define APP_CORE_SCHEDULED_MESSAGE_H

#include <stdint.h>

#include "core/scheduled_state.h"

/* A text message kept on this computer until it is due, then sent. */
typedef struct ScheduledMessage {
    char           id[64];
    char           chat_jid[128];
    char          *text;           /* owned */
    char          *mentions;       /* owned "jid\tuser" lines, may be NULL */
    int64_t        due_at;         /* epoch seconds */
    int64_t        created_at;
    ScheduledState state;
} ScheduledMessage;

void scheduled_message_init(ScheduledMessage *message);
void scheduled_message_dispose(ScheduledMessage *message);
/* Disposes each and frees the array. */
void scheduled_message_array_free(ScheduledMessage *items, int count);

#endif
