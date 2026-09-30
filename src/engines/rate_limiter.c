#include "engines/rate_limiter.h"

#include <string.h>

void rate_limiter_init(RateLimiter *r) { memset(r, 0, sizeof(*r)); }

int rate_limiter_take(RateLimiter *r, int per_minute, int64_t now_ms, int *retry_after_s) {
    if (per_minute < 1) per_minute = 1;
    double per_ms = per_minute / 60000.0;
    if (!r->started) {
        r->tokens = per_minute;
        r->last_ms = now_ms;
        r->started = 1;
    }
    if (now_ms > r->last_ms) r->tokens += (double)(now_ms - r->last_ms) * per_ms;
    r->last_ms = now_ms;
    if (r->tokens > per_minute) r->tokens = per_minute;
    if (r->tokens >= 1.0) {
        r->tokens -= 1.0;
        if (retry_after_s) *retry_after_s = 0;
        return 1;
    }
    if (retry_after_s) *retry_after_s = (int)((1.0 - r->tokens) / per_ms / 1000.0) + 1;
    return 0;
}
