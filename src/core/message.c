#include "core/message.h"
#include "utilities/str_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void message_init(Message *msg) {
    memset(msg, 0, sizeof(*msg));
}

void message_dispose(Message *msg) {
    if (!msg) return;
    free(msg->text);
    free(msg->media_ref);
    free(msg->quoted_text);
    free(msg->thumbnail);
    free(msg->mentions);
    free(msg->link);
    msg->mentions = NULL;
    msg->link = NULL;
    msg->text = NULL;
    msg->media_ref = NULL;
    msg->quoted_text = NULL;
    msg->thumbnail = NULL;
    msg->thumbnail_len = 0;
}

void message_set_text(Message *msg, const char *text) {
    free(msg->text);
    msg->text = str_dup(text);
}

void message_set_media_ref(Message *msg, const char *ref) {
    free(msg->media_ref);
    msg->media_ref = (ref && ref[0]) ? str_dup(ref) : NULL;
}

void message_set_quoted_text(Message *msg, const char *text) {
    free(msg->quoted_text);
    msg->quoted_text = (text && text[0]) ? str_dup(text) : NULL;
}

void message_set_mentions(Message *msg, const char *mentions) {
    free(msg->mentions);
    msg->mentions = (mentions && mentions[0]) ? str_dup(mentions) : NULL;
}

void message_set_link(Message *msg, LinkPreview *link) {
    free(msg->link);
    msg->link = link;
}

void message_set_thumbnail(Message *msg, const unsigned char *data, int len) {
    free(msg->thumbnail);
    msg->thumbnail = NULL;
    msg->thumbnail_len = 0;
    if (!data || len <= 0) return;
    msg->thumbnail = malloc((size_t)len);
    if (!msg->thumbnail) return;
    memcpy(msg->thumbnail, data, (size_t)len);
    msg->thumbnail_len = len;
}

void message_copy(Message *dst, const Message *src) {
    *dst = *src;
    dst->text = str_dup(src->text);
    dst->media_ref = str_dup(src->media_ref);
    dst->quoted_text = str_dup(src->quoted_text);
    dst->mentions = str_dup(src->mentions);
    dst->link = link_preview_copy(src->link);
    dst->thumbnail = NULL;
    dst->thumbnail_len = 0;
    message_set_thumbnail(dst, src->thumbnail, src->thumbnail_len);
}

void message_array_free(Message *items, int count) {
    if (!items) return;
    for (int i = 0; i < count; i++) message_dispose(&items[i]);
    free(items);
}

static const char *type_label(MessageType type) {
    switch (type) {
        case MESSAGE_TYPE_IMAGE:    return "Photo";
        case MESSAGE_TYPE_VIDEO:    return "Video";
        case MESSAGE_TYPE_AUDIO:    return "Voice message";
        case MESSAGE_TYPE_DOCUMENT: return "Document";
        case MESSAGE_TYPE_STICKER:  return "Sticker";
        case MESSAGE_TYPE_OTHER:    return "Message";
        default:                    return "";
    }
}

void message_preview(const Message *msg, char *out, unsigned long size) {
    const char *text = msg->text ? msg->text : "";
    if (msg->type == MESSAGE_TYPE_TEXT) {
        str_copy(out, size, text);
    } else if (text[0]) {
        snprintf(out, size, "%s %s", message_type_emoji(msg->type), text);
    } else {
        snprintf(out, size, "%s %s", message_type_emoji(msg->type), type_label(msg->type));
    }
    for (char *p = out; *p; p++) if (*p == '\n' || *p == '\r' || *p == '\t') *p = ' ';
}
