#include "clients/tui/quoted_status.h"
#include "utilities/str_util.h"

#include <string.h>

void quoted_status_load(QuotedStatus *q, const StatusSource *source, const char *status_id) {
    memset(q, 0, sizeof(*q));
    status_update_init(&q->update);
    message_init(&q->picture);
    if (!source || !source->find || !status_id || !status_id[0]) return;
    if (source->find(source->ctx, status_id, &q->update) != 0) {
        status_update_dispose(&q->update);
        status_update_init(&q->update);
        return;
    }
    q->found = 1;
    str_copy(q->picture.id, sizeof(q->picture.id), q->update.id);
    q->picture.type = q->update.type;
    str_copy(q->picture.media_path, sizeof(q->picture.media_path), q->update.media_path);
    if (q->update.thumbnail && q->update.thumbnail_len > 0) {
        message_set_thumbnail(&q->picture, q->update.thumbnail, q->update.thumbnail_len);
    }
}

void quoted_status_dispose(QuotedStatus *q) {
    status_update_dispose(&q->update);
    message_dispose(&q->picture);
    memset(q, 0, sizeof(*q));
}

int quoted_status_has_picture(const QuotedStatus *q) {
    return q->found && (q->update.type == MESSAGE_TYPE_IMAGE || q->update.type == MESSAGE_TYPE_VIDEO);
}
