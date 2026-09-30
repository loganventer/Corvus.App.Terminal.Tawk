#ifndef APP_CONTRACTS_I_RECEIPT_STORE_H
#define APP_CONTRACTS_I_RECEIPT_STORE_H

#include <stdint.h>

#include "core/receipt.h"
#include "core/receipt_kind.h"

/* Who received, read and played the messages you sent, and when. */
typedef struct IReceiptStore {
    void *ctx;
    /* Records one receipt; reading implies delivery, playing implies both.
     * The earliest time of each kind is kept. */
    int  (*put)(struct IReceiptStore *self, const char *message_id, const char *jid, ReceiptKind kind, int64_t at);
    /* The recipients of a message, most recently read first; returns how many. */
    int  (*list)(struct IReceiptStore *self, const char *message_id, Receipt *out, int max);
    /* Moves receipts from `from` to `to` (alias merge). */
    int  (*reassign_jid)(struct IReceiptStore *self, const char *from, const char *to);
    void (*destroy)(struct IReceiptStore *self);
} IReceiptStore;

#endif
