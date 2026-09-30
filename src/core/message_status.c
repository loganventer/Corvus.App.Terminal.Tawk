#include "core/message_status.h"

#include <string.h>

MessageStatus message_status_parse(const char *name) {
    if (!name) return MESSAGE_STATUS_PENDING;
    if (strcmp(name, "sent") == 0) return MESSAGE_STATUS_SENT;
    if (strcmp(name, "delivered") == 0) return MESSAGE_STATUS_DELIVERED;
    if (strcmp(name, "read") == 0) return MESSAGE_STATUS_READ;
    if (strcmp(name, "failed") == 0) return MESSAGE_STATUS_FAILED;
    return MESSAGE_STATUS_PENDING;
}

const char *message_status_ticks(MessageStatus status) {
    switch (status) {
        case MESSAGE_STATUS_PENDING:   return "\xE2\x97\xB7";         /* ◷ */
        case MESSAGE_STATUS_SENT:      return "\xE2\x9C\x93";         /* ✓ */
        case MESSAGE_STATUS_DELIVERED: return "\xE2\x9C\x93\xE2\x9C\x93";
        case MESSAGE_STATUS_READ:      return "\xE2\x9C\x93\xE2\x9C\x93";
        case MESSAGE_STATUS_FAILED:    return "\xE2\x9C\x97";         /* ✗ */
    }
    return "";
}
