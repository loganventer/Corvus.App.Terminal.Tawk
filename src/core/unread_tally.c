#include "core/unread_tally.h"

#include <stdio.h>
#include <string.h>

int unread_tally_total(const UnreadTally *tally) {
    int total = 0;
    for (int i = 0; i < MESSAGE_TYPE_COUNT; i++) total += tally->counts[i];
    return total;
}

void unread_tally_format(const UnreadTally *tally, char *out, unsigned long size) {
    size_t used = 0;
    out[0] = '\0';
    for (int i = 0; i < MESSAGE_TYPE_COUNT && used < size; i++) {
        if (tally->counts[i] <= 0) continue;
        int n = snprintf(out + used, size - used, "%s%s %d",
                         used ? "  " : "", message_type_emoji((MessageType)i), tally->counts[i]);
        if (n < 0) break;
        used += (size_t)n;
    }
}
