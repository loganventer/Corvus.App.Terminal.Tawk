#ifndef APP_CORE_INCOMING_CALL_H
#define APP_CORE_INCOMING_CALL_H

#include <stdint.h>

/* A voice call ringing on the account. */
typedef struct IncomingCall {
    char    id[64];
    char    from[128];
    int64_t since_ms;        /* when it started ringing (monotonic) */
} IncomingCall;

#endif
