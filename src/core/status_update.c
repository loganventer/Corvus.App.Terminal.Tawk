#include "core/status_update.h"

#include <stdlib.h>
#include <string.h>

void status_update_init(StatusUpdate *u) {
    memset(u, 0, sizeof(*u));
}

void status_update_dispose(StatusUpdate *u) {
    if (!u) return;
    free(u->text);
    free(u->media_ref);
    free(u->thumbnail);
    u->text = u->media_ref = NULL;
    u->thumbnail = NULL;
    u->thumbnail_len = 0;
}

/* Disposes each item. The array itself belongs to the caller (often on the
 * stack), so it is not freed here. */
void status_update_array_free(StatusUpdate *items, int count) {
    if (!items) return;
    for (int i = 0; i < count; i++) status_update_dispose(&items[i]);
}
