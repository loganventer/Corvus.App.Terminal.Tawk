#ifndef APP_MANAGERS_CONNECTION_HEALTH_H
#define APP_MANAGERS_CONNECTION_HEALTH_H

#include <stdint.h>

#include "engines/circuit_state.h"

/* What the UI needs to explain an outage. */
typedef struct ConnectionHealth {
    int          available;      /* connected and usable */
    int          show_overlay;   /* outage the user should be told about */
    int          will_retry;     /* an automatic retry is scheduled */
    int64_t      retry_in_ms;
    int          attempt;
    CircuitState breaker;
    char         title[96];
    char         detail[256];
} ConnectionHealth;

#endif
