#ifndef APP_CLIENTS_TUI_QUOTED_STATUS_H
#define APP_CLIENTS_TUI_QUOTED_STATUS_H

#include "clients/tui/status_source.h"
#include "core/message.h"
#include "core/status_update.h"

/* The status a reply answers, as the conversation shows it: the status
 * itself, and a message standing in for its picture (the thumbnail cache
 * works on messages). `found` is 0 when the status is no longer kept. */
typedef struct QuotedStatus {
    int          found;
    StatusUpdate update;
    Message      picture;
} QuotedStatus;

/* Looks `status_id` up through `source` (which may be NULL). */
void quoted_status_load(QuotedStatus *quoted, const StatusSource *source, const char *status_id);
void quoted_status_dispose(QuotedStatus *quoted);
/* True for a photo or video status. */
int  quoted_status_has_picture(const QuotedStatus *quoted);

#endif
