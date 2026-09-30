#ifndef APP_CORE_READ_REQUEST_H
#define APP_CORE_READ_REQUEST_H

#include <stdint.h>

#include "core/read_item.h"

#define READ_REQUEST_MAX 64

/* Everything a backend needs to mark a chat as read. Two things happen:
 * read receipts to the senders (only with `send_receipts`), and always the
 * account-wide "read" mark that clears the chat's unread badge on your
 * phone and other linked devices. The latter needs the chat's newest
 * message. */
typedef struct ReadRequest {
    char     chat_jid[128];
    ReadItem items[READ_REQUEST_MAX];   /* the unread incoming messages, newest first */
    int      count;
    int      send_receipts;
    char     last_id[64];               /* the chat's newest message, for the read mark */
    char     last_sender[128];
    int      last_from_me;
    int64_t  last_timestamp;
} ReadRequest;

#endif
