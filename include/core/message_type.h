#ifndef APP_CORE_MESSAGE_TYPE_H
#define APP_CORE_MESSAGE_TYPE_H

typedef enum MessageType {
    MESSAGE_TYPE_TEXT = 0,
    MESSAGE_TYPE_IMAGE,
    MESSAGE_TYPE_VIDEO,
    MESSAGE_TYPE_AUDIO,
    MESSAGE_TYPE_DOCUMENT,
    MESSAGE_TYPE_STICKER,
    MESSAGE_TYPE_OTHER,
    MESSAGE_TYPE_COUNT
} MessageType;

MessageType message_type_parse(const char *name);
const char *message_type_name(MessageType type);
/* Emoji used in the terminal title and in chat bubbles. */
const char *message_type_emoji(MessageType type);
/* True for media that can be downloaded and opened in a viewer. */
int         message_type_is_openable(MessageType type);

#endif
