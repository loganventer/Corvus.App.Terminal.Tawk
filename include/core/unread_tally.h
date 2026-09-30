#ifndef APP_CORE_UNREAD_TALLY_H
#define APP_CORE_UNREAD_TALLY_H

#include "core/message_type.h"

/* Count of new notifications per message type, shown in the terminal title. */
typedef struct UnreadTally {
    int counts[MESSAGE_TYPE_COUNT];
} UnreadTally;

int  unread_tally_total(const UnreadTally *tally);
/* "💬 3  🖼 1  🎬 2"; empty when there is nothing new. */
void unread_tally_format(const UnreadTally *tally, char *out, unsigned long size);

#endif
