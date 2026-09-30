#ifndef APP_CORE_CONTROL_INBOUND_H
#define APP_CORE_CONTROL_INBOUND_H

#include "core/control_inbound_kind.h"

/* Something that happened on the control socket since it was last polled. */
typedef struct ControlInbound {
    ControlInboundKind kind;
    int                conn;     /* which client, stable while it stays connected */
    char              *line;     /* owned, without the newline; LINE only */
} ControlInbound;

void control_inbound_dispose(ControlInbound *inbound);

#endif
