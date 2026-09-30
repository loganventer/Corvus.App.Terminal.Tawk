#ifndef APP_CONTRACTS_I_REACTION_STORE_H
#define APP_CONTRACTS_I_REACTION_STORE_H

#include <stddef.h>

#include "core/reaction.h"

typedef struct IReactionStore {
    void *ctx;
    /* One reaction per sender per message; an empty emoji removes it. */
    int  (*put)(struct IReactionStore *self, const char *message_id, const char *sender, const char *emoji);
    /* "👍 2  ❤ 1" for a message, or "" when it has none. */
    void (*summary)(struct IReactionStore *self, const char *message_id, char *out, size_t size);
    /* Who reacted to a message and how; returns how many were written. */
    int  (*list)(struct IReactionStore *self, const char *message_id, Reaction *out, int max);
    /* Moves reactions made by `from` to `to` (alias merge). */
    int  (*reassign_sender)(struct IReactionStore *self, const char *from, const char *to);
    void (*destroy)(struct IReactionStore *self);
} IReactionStore;

#endif
