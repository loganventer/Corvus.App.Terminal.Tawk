#ifndef APP_CORE_STATUS_UPDATE_H
#define APP_CORE_STATUS_UPDATE_H

#include <stdint.h>

#include "core/message_type.h"

/* One status someone posted (you included), shown for a day. */
typedef struct StatusUpdate {
    char           id[64];
    char           author_jid[128];
    char           author_name[128];   /* their push name, "" when unknown */
    MessageType    type;               /* TEXT, IMAGE or VIDEO; others are shown as a line of text */
    char          *text;               /* owned: the words, or a caption; may be NULL */
    char          *media_ref;          /* owned opaque download reference; may be NULL */
    char           media_path[512];    /* "" until downloaded */
    unsigned char *thumbnail;          /* owned JPEG preview; may be NULL */
    int            thumbnail_len;
    uint32_t       background_argb;    /* text statuses; 0 when not given */
    int64_t        timestamp;          /* epoch seconds */
    int            from_me;
    int            viewed;
} StatusUpdate;

void status_update_init(StatusUpdate *update);
void status_update_dispose(StatusUpdate *update);
void status_update_array_free(StatusUpdate *items, int count);

#endif
