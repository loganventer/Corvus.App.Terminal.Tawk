#ifndef APP_CORE_CONTROL_INBOUND_KIND_H
#define APP_CORE_CONTROL_INBOUND_KIND_H

typedef enum ControlInboundKind {
    CONTROL_INBOUND_OPENED = 0,     /* a client connected */
    CONTROL_INBOUND_LINE,           /* one line from it */
    CONTROL_INBOUND_CLOSED          /* it went away, or broke the rules */
} ControlInboundKind;

#endif
