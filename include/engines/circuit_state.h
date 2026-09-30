#ifndef APP_ENGINES_CIRCUIT_STATE_H
#define APP_ENGINES_CIRCUIT_STATE_H

typedef enum CircuitState {
    CIRCUIT_CLOSED = 0,   /* healthy, requests flow */
    CIRCUIT_OPEN,         /* failing, requests are refused until the cooldown ends */
    CIRCUIT_HALF_OPEN     /* one trial request allowed */
} CircuitState;

const char *circuit_state_name(CircuitState state);

#endif
