#ifndef APP_CORE_LIVE_MESSAGE_REF_H
#define APP_CORE_LIVE_MESSAGE_REF_H

#include <stdint.h>

#include "core/live_kind.h"

/* A message that just arrived or was just sent, or one of yours that was
 * just read, reacted to, edited, deleted or sent on schedule, numbered in
 * the order it happened. */
typedef struct LiveMessageRef {
    uint64_t seq;
    LiveKind kind;
    char     id[64];
    char     chat_jid[128];
    char     who[128];          /* who read it, reacted, edited or deleted */
    char     detail[64];        /* REACTION: the emoji, "" when taken back */
    int64_t  at;                /* when, in seconds */
} LiveMessageRef;

#endif
