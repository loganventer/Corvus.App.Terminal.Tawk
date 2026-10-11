#ifndef APP_CONTRACTS_I_SUMMARY_STORE_H
#define APP_CONTRACTS_I_SUMMARY_STORE_H

#include <stdint.h>

#include "core/summary.h"

/* The TL;DR summaries of long messages, one per message. */
typedef struct ISummaryStore {
    void *ctx;
    /* Keeps `summary`, replacing the one this message already has. */
    int  (*save)(struct ISummaryStore *self, const Summary *summary);
    /* Fills `out` (the caller disposes it) and returns 0, or -1 when the message has none. */
    int  (*find)(struct ISummaryStore *self, const char *message_id, Summary *out);
    int  (*remove)(struct ISummaryStore *self, const char *message_id);
    /* Remembers that an agent said this message needs no summary (`reason`, `source`, when), so it is not
     * asked for again. Returns 0, or -1 when it could not be kept. */
    int  (*decline)(struct ISummaryStore *self, const char *message_id, const char *reason, const char *source, int64_t at);
    /* Whether the message was declined: 1, or 0. */
    int  (*declined)(struct ISummaryStore *self, const char *message_id);
    void (*destroy)(struct ISummaryStore *self);
} ISummaryStore;

#endif
