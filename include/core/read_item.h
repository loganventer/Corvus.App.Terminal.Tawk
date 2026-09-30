#ifndef APP_CORE_READ_ITEM_H
#define APP_CORE_READ_ITEM_H

/* One incoming message being marked as read: read receipts in groups go
 * to its sender. */
typedef struct ReadItem {
    char id[64];
    char sender[128];
} ReadItem;

#endif
