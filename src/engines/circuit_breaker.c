#include "engines/circuit_breaker.h"

void circuit_breaker_init(CircuitBreaker *b, int threshold, int cooldown_s) {
    b->state = CIRCUIT_CLOSED;
    b->failures = 0;
    b->threshold = threshold > 0 ? threshold : 5;
    b->cooldown_ms = (int64_t)(cooldown_s > 0 ? cooldown_s : 60) * 1000;
    b->opened_at_ms = 0;
}

int circuit_breaker_allow(CircuitBreaker *b, int64_t now_ms) {
    if (b->state == CIRCUIT_OPEN && now_ms - b->opened_at_ms >= b->cooldown_ms) {
        b->state = CIRCUIT_HALF_OPEN;
    }
    return b->state != CIRCUIT_OPEN;
}

void circuit_breaker_record_success(CircuitBreaker *b) {
    b->state = CIRCUIT_CLOSED;
    b->failures = 0;
}

void circuit_breaker_record_failure(CircuitBreaker *b, int64_t now_ms) {
    b->failures++;
    /* Only the move into OPEN starts the cooldown. Failures reported while
     * already open (a backend repeating the same outage) must not push the
     * trial further away, or a steady trickle keeps the circuit open forever. */
    if (b->state == CIRCUIT_OPEN) return;
    if (b->state == CIRCUIT_HALF_OPEN || b->failures >= b->threshold) {
        b->state = CIRCUIT_OPEN;
        b->opened_at_ms = now_ms;
    }
}

int64_t circuit_breaker_remaining_ms(const CircuitBreaker *b, int64_t now_ms) {
    if (b->state != CIRCUIT_OPEN) return 0;
    int64_t left = b->cooldown_ms - (now_ms - b->opened_at_ms);
    return left > 0 ? left : 0;
}
