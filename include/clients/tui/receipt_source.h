#ifndef APP_CLIENTS_TUI_RECEIPT_SOURCE_H
#define APP_CLIENTS_TUI_RECEIPT_SOURCE_H

#include "core/account_id.h"
#include "core/message.h"
#include "core/receipt.h"

/* Where the conversation finds how a message you sent was received, without
 * knowing who keeps the receipts. `list` fills `out` with up to `max`
 * receipts of the message and returns how many (0 when there are none).
 * `owner` is the account the message belongs to in a chat merged across
 * accounts, ACCOUNT_ID_NONE anywhere else. */
typedef struct ReceiptSource {
    void *ctx;
    int  (*list)(void *ctx, const Message *message, AccountId owner, Receipt *out, int max);
} ReceiptSource;

#endif
