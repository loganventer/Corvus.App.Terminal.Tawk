#ifndef APP_CORE_MESSAGE_H
#define APP_CORE_MESSAGE_H

#include <stdint.h>

#include "core/link_preview.h"
#include "core/message_status.h"
#include "core/message_type.h"

typedef struct Message {
    char          id[64];
    char          chat_jid[128];
    char          sender_jid[128];
    char          sender_name[128];
    char         *text;        /* owned, may be NULL */
    char         *media_ref;   /* owned opaque download reference, may be NULL */
    char          media_path[512];
    MessageType   type;
    MessageStatus status;
    int64_t       timestamp;
    int           duration_s;  /* audio length, 0 when unknown */
    int           from_me;
    char          quoted_id[64];      /* reply: the message being answered */
    char          quoted_sender[128];
    char         *quoted_text;        /* owned, may be NULL */
    unsigned char *thumbnail;         /* owned JPEG preview, may be NULL */
    int           thumbnail_len;
    char          reactions[96];      /* summary such as "👍 2  ❤ 1", filled on load */
    int           edited;             /* the sender changed the text */
    int           deleted;            /* deleted for everyone */
    char         *mentions;           /* owned "jid\tuser" lines of the people mentioned, may be NULL */
    int           mentions_me;        /* you are one of them */
    int           forwarded;          /* marked as forwarded by the sender */
    int           quoted_status;      /* the reply answers a status (quoted_id is the status's id) */
    LinkPreview  *link;               /* owned card for a web address in the text, may be NULL */
    uint32_t      background_argb;    /* a text status's background; 0 when not given, never stored */
} Message;

void message_init(Message *msg);
/* Frees owned fields; the struct itself is not freed. */
void message_dispose(Message *msg);
void message_set_text(Message *msg, const char *text);
void message_set_media_ref(Message *msg, const char *ref);
void message_set_quoted_text(Message *msg, const char *text);
void message_set_mentions(Message *msg, const char *mentions);
/* Takes ownership of `link` (may be NULL). */
void message_set_link(Message *msg, LinkPreview *link);
/* Takes a copy of the JPEG bytes. */
void message_set_thumbnail(Message *msg, const unsigned char *data, int len);
/* Deep copy; dst must be initialised or disposed. */
void message_copy(Message *dst, const Message *src);
void message_array_free(Message *items, int count);
/* The text shown in chat lists and notifications, e.g. "🖼 Photo". */
void message_preview(const Message *msg, char *out, unsigned long size);

#endif
