#include "core/event.h"

#include <stdlib.h>
#include <string.h>

void event_init(Event *evt, EventType type) {
    memset(evt, 0, sizeof(*evt));
    evt->type = type;
}

void event_dispose(Event *evt) {
    if (!evt) return;
    message_dispose(&evt->message);
    free(evt->qr_ascii);
    evt->qr_ascii = NULL;
    if (evt->profile) { contact_profile_dispose(evt->profile); free(evt->profile); evt->profile = NULL; }
    free(evt->list);
    evt->list = NULL;
}
