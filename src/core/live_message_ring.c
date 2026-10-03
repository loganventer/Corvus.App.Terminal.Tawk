#include "core/live_message_ring.h"
#include "utilities/str_util.h"

#include <string.h>

void live_message_ring_init(LiveMessageRing *ring) {
    memset(ring, 0, sizeof(*ring));
    ring->next_seq = 1;
}

void live_message_ring_push(LiveMessageRing *ring, const char *id, const char *chat_jid) {
    LiveMessageRef *slot = &ring->items[ring->next_seq % LIVE_MESSAGE_RING_SIZE];
    memset(slot, 0, sizeof(*slot));
    slot->seq = ring->next_seq++;
    slot->kind = LIVE_KIND_MESSAGE;
    str_copy(slot->id, sizeof(slot->id), id ? id : "");
    str_copy(slot->chat_jid, sizeof(slot->chat_jid), chat_jid ? chat_jid : "");
}

void live_message_ring_note(LiveMessageRing *ring, LiveKind kind, const char *id, const char *chat_jid,
                            const char *who, const char *detail, int64_t at) {
    live_message_ring_push(ring, id, chat_jid);
    LiveMessageRef *slot = &ring->items[(ring->next_seq - 1) % LIVE_MESSAGE_RING_SIZE];
    slot->kind = kind;
    str_copy(slot->who, sizeof(slot->who), who ? who : "");
    str_copy(slot->detail, sizeof(slot->detail), detail ? detail : "");
    slot->at = at;
}

uint64_t live_message_ring_last(const LiveMessageRing *ring) {
    return ring->next_seq > 1 ? ring->next_seq - 1 : 0;
}

int live_message_ring_since(const LiveMessageRing *ring, uint64_t after, LiveMessageRef *out, int max) {
    uint64_t last = live_message_ring_last(ring);
    uint64_t first = last >= LIVE_MESSAGE_RING_SIZE ? last - LIVE_MESSAGE_RING_SIZE + 1 : 1;
    if (after + 1 > first) first = after + 1;
    int n = 0;
    for (uint64_t seq = first; seq <= last && n < max; seq++) out[n++] = ring->items[seq % LIVE_MESSAGE_RING_SIZE];
    return n;
}
