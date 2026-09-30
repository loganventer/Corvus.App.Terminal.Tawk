#ifndef APP_ENGINES_CIRCUIT_BREAKER_H
#define APP_ENGINES_CIRCUIT_BREAKER_H

#include <stdint.h>

#include "engines/circuit_state.h"

typedef struct CircuitBreaker {
    CircuitState state;
    int          failures;
    int          threshold;
    int64_t      cooldown_ms;
    int64_t      opened_at_ms;
} CircuitBreaker;

void circuit_breaker_init(CircuitBreaker *breaker, int threshold, int cooldown_s);
/* True when a request may proceed; moves OPEN to HALF_OPEN after the cooldown. */
int  circuit_breaker_allow(CircuitBreaker *breaker, int64_t now_ms);
void circuit_breaker_record_success(CircuitBreaker *breaker);
void circuit_breaker_record_failure(CircuitBreaker *breaker, int64_t now_ms);
/* Milliseconds until an open circuit allows a trial, 0 otherwise. */
int64_t circuit_breaker_remaining_ms(const CircuitBreaker *breaker, int64_t now_ms);

#endif
