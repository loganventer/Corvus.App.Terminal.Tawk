#ifndef APP_CORE_LIVE_MESSAGE_RING_H
#define APP_CORE_LIVE_MESSAGE_RING_H

#include <stdint.h>

#include "core/live_message_ref.h"

#define LIVE_MESSAGE_RING_SIZE 256

/* The last few hundred messages that arrived or were sent while tawk ran,
 * so a reader that looks now and then (the control socket) sees each once.
 * The oldest are overwritten; sequence numbers start at 1. */
typedef struct LiveMessageRing {
    LiveMessageRef items[LIVE_MESSAGE_RING_SIZE];
    uint64_t       next_seq;
} LiveMessageRing;

void     live_message_ring_init(LiveMessageRing *ring);
void     live_message_ring_push(LiveMessageRing *ring, const char *id, const char *chat_jid);
/* `who` read your message `id` in `chat_jid` at `at`. */
void     live_message_ring_push_read(LiveMessageRing *ring, const char *id, const char *chat_jid, const char *who, int64_t at);
/* Copies up to `max` entries newer than `after`, oldest first; returns how many. */
int      live_message_ring_since(const LiveMessageRing *ring, uint64_t after, LiveMessageRef *out, int max);
/* The sequence number of the newest entry, 0 when there is none. */
uint64_t live_message_ring_last(const LiveMessageRing *ring);

#endif
