#ifndef APP_CONTRACTS_I_CONTROL_TRANSPORT_H
#define APP_CONTRACTS_I_CONTROL_TRANSPORT_H

#include "core/control_inbound.h"

/* The listening end of the control socket. Never blocks: the owner polls
 * it once a frame from the UI thread. */
typedef struct IControlTransport {
    void *ctx;
    /* Starts listening at `path`; returns 0, or -1 with the reason in `why`. */
    int  (*listen)(struct IControlTransport *self, const char *path, char *why, unsigned long why_size);
    /* Stops listening and drops every client. */
    void (*stop)(struct IControlTransport *self);
    /* What happened since the last poll, up to `max`; the caller disposes each. */
    int  (*poll)(struct IControlTransport *self, ControlInbound *out, int max);
    /* Queues one line (a newline is added); returns -1 when the client is gone. */
    int  (*send)(struct IControlTransport *self, int conn, const char *line);
    void (*close_conn)(struct IControlTransport *self, int conn);
    void (*destroy)(struct IControlTransport *self);
} IControlTransport;

#endif
