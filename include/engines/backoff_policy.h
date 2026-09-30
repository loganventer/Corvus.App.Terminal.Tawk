#ifndef APP_ENGINES_BACKOFF_POLICY_H
#define APP_ENGINES_BACKOFF_POLICY_H

#include <stdint.h>

typedef struct BackoffPolicy {
    int initial_ms;
    int max_ms;
} BackoffPolicy;

/* Exponential backoff with full jitter: random in [0, min(max, initial * 2^attempt)].
 * The floor is half the cap so retries never fire back to back. */
int64_t backoff_policy_delay_ms(const BackoffPolicy *policy, int attempt);

#endif
