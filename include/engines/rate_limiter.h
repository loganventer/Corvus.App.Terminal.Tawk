#ifndef APP_ENGINES_RATE_LIMITER_H
#define APP_ENGINES_RATE_LIMITER_H

#include <stdint.h>

/* A token bucket: up to `per_minute` at once, refilled evenly over a minute. */
typedef struct RateLimiter {
    double  tokens;
    int64_t last_ms;
    int     started;
} RateLimiter;

void rate_limiter_init(RateLimiter *limiter);
/* Takes one token; returns 1 when there was one, else 0 with the seconds
 * until the next in *retry_after_s. */
int  rate_limiter_take(RateLimiter *limiter, int per_minute, int64_t now_ms, int *retry_after_s);

#endif
