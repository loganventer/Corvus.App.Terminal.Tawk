#ifndef APP_CORE_LIVE_MESSAGE_REF_H
#define APP_CORE_LIVE_MESSAGE_REF_H

#include <stdint.h>

/* A message that just arrived or was just sent, numbered in arrival order. */
typedef struct LiveMessageRef {
    uint64_t seq;
    char     id[64];
    char     chat_jid[128];
} LiveMessageRef;

#endif
