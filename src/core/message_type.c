#include "core/icon_glyphs.h"
#include "core/message_type.h"

#include <string.h>

static const char *const NAMES[MESSAGE_TYPE_COUNT] = {
    "text", "image", "video", "audio", "document", "sticker", "other"
};

static const char *const EMOJI[MESSAGE_TYPE_COUNT] = {
    "\xF0\x9F\x92\xAC", /* 💬 */
    "\xF0\x9F\x93\xB7", /* 📷 two columns everywhere, unlike 🖼 */
    "\xF0\x9F\x8E\xAC", /* 🎬 */
    "\xF0\x9F\x94\x8A", /* 🔊 */
    "\xF0\x9F\x93\x84", /* 📄 */
    "\xF0\x9F\x94\x96", /* 🔖 two columns everywhere, unlike 🏷 */
    ICON_FILE           /* other files: a plain page */
};

MessageType message_type_parse(const char *name) {
    if (!name) return MESSAGE_TYPE_OTHER;
    for (int i = 0; i < MESSAGE_TYPE_COUNT; i++) {
        if (strcmp(name, NAMES[i]) == 0) return (MessageType)i;
    }
    return MESSAGE_TYPE_OTHER;
}

const char *message_type_name(MessageType type) {
    return (type >= 0 && type < MESSAGE_TYPE_COUNT) ? NAMES[type] : "other";
}

const char *message_type_emoji(MessageType type) {
    return (type >= 0 && type < MESSAGE_TYPE_COUNT) ? EMOJI[type] : EMOJI[MESSAGE_TYPE_OTHER];
}

int message_type_is_openable(MessageType type) {
    return type == MESSAGE_TYPE_IMAGE || type == MESSAGE_TYPE_VIDEO ||
           type == MESSAGE_TYPE_AUDIO || type == MESSAGE_TYPE_DOCUMENT ||
           type == MESSAGE_TYPE_STICKER;
}
