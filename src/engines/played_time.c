#include "engines/played_time.h"

int played_time_applies(MessageType type) {
    return type == MESSAGE_TYPE_AUDIO || type == MESSAGE_TYPE_VIDEO;
}

int64_t played_time_first(const Receipt *receipts, int count) {
    int64_t first = 0;
    for (int i = 0; i < count; i++) {
        int64_t at = receipts[i].played_at;
        if (at > 0 && (first == 0 || at < first)) first = at;
    }
    return first;
}
