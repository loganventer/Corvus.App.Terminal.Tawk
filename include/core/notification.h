#ifndef APP_CORE_NOTIFICATION_H
#define APP_CORE_NOTIFICATION_H

#include "core/message_type.h"

typedef struct Notification {
    char        chat_jid[128];
    char        title[128];
    char        body[256];
    MessageType type;
    int         is_group;
    char        tone[256];    /* chat-specific sound; "" default, "none" silent */
} Notification;

#endif
