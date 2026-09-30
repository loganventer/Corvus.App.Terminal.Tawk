#ifndef APP_MANAGERS_MANAGER_CHANGES_H
#define APP_MANAGERS_MANAGER_CHANGES_H

#include "core/message_type.h"

/* What changed during one messaging_manager_tick, so the client redraws only what it must. */
typedef struct ManagerChanges {
    int         chats;
    int         profiles;           /* details, pictures or the block list changed */
    int         statuses;           /* a status arrived, was deleted or its photo or video downloaded */
    int         messages;
    int         auth;
    int         connection;
    int         notified;           /* notifications raised this tick */
    int         live_message;       /* any live incoming message (wakes the screensaver) */
    char        media_id[64];       /* a requested download finished */
    char        media_path[512];
    MessageType media_type;
    char        error[256];         /* user-facing error to show briefly */
} ManagerChanges;

#endif
