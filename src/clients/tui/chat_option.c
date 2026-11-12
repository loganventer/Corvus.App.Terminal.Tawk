#include "clients/tui/chat_option.h"

static const char *const LABELS[CHAT_OPTION_COUNT] = {
    "\xF0\x9F\x94\x95  Mute for 8 hours",
    "\xF0\x9F\x94\x95  Mute for 1 week",
    "\xF0\x9F\x94\x95  Mute always",
    "\xF0\x9F\x94\x94  Unmute",
    "\xF0\x9F\x93\x8C  Pin chat",
    "\xF0\x9F\x93\x8C  Unpin chat",
    "\xF0\x9F\x97\x84  Archive chat",
    "\xF0\x9F\x97\x84  Unarchive chat",
    "\xF0\x9F\x8E\xA8  Chat theme\xE2\x80\xA6",
    "\xF0\x9F\x8E\xB5  Notification tone: choose file\xE2\x80\xA6",
    "\xF0\x9F\x94\x87  Notification tone: none",
    "\xF0\x9F\x8E\xB5  Notification tone: default",
    "\xE2\x9C\x8F  Clear draft",
    "\xF0\x9F\x94\x92  Make private (Touch ID or password to open)",
    "\xF0\x9F\x94\x93  Make public",
    "\xF0\x9F\x97\x91  Delete chat\xE2\x80\xA6",
    "\xE2\x84\xB9  Contact info\xE2\x80\xA6",
};

const char *chat_option_label(ChatOption option) {
    return (option >= 0 && option < CHAT_OPTION_COUNT) ? LABELS[option] : "";
}
