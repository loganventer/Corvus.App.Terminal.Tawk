#ifndef APP_CORE_MESSAGE_STATUS_H
#define APP_CORE_MESSAGE_STATUS_H

typedef enum MessageStatus {
    MESSAGE_STATUS_PENDING = 0,
    MESSAGE_STATUS_SENT,
    MESSAGE_STATUS_DELIVERED,
    MESSAGE_STATUS_READ,
    MESSAGE_STATUS_FAILED
} MessageStatus;

MessageStatus message_status_parse(const char *name);
/* WhatsApp-style ticks: clock, single, double, double (read), cross. */
const char   *message_status_ticks(MessageStatus status);

#endif
