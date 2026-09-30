#ifndef APP_MANAGERS_CHAT_TALLY_H
#define APP_MANAGERS_CHAT_TALLY_H

#include "core/unread_tally.h"

/* New-notification counts for one chat since it was last opened. */
typedef struct ChatTally {
    char        jid[128];
    UnreadTally tally;
} ChatTally;

#endif
