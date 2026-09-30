#include "core/scheduled_message.h"

#include <stdlib.h>
#include <string.h>

void scheduled_message_init(ScheduledMessage *s) { memset(s, 0, sizeof(*s)); }

void scheduled_message_dispose(ScheduledMessage *s) {
    if (!s) return;
    free(s->text);
    free(s->mentions);
    s->text = s->mentions = NULL;
}

void scheduled_message_array_free(ScheduledMessage *items, int count) {
    if (!items) return;
    for (int i = 0; i < count; i++) scheduled_message_dispose(&items[i]);
    free(items);
}
