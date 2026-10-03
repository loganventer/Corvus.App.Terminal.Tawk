#include "engines/hourly_quota.h"

#include <string.h>

#define HOUR_MS (60 * 60 * 1000)

void hourly_quota_init(HourlyQuota *q) { memset(q, 0, sizeof(*q)); }

int hourly_quota_take(HourlyQuota *q, int per_hour, int64_t now_ms, int *retry_after_s) {
    if (per_hour > HOURLY_QUOTA_MAX) per_hour = HOURLY_QUOTA_MAX;
    int lapsed = 0;
    while (lapsed < q->count && now_ms - q->taken_ms[lapsed] >= HOUR_MS) lapsed++;
    if (lapsed) {
        memmove(q->taken_ms, q->taken_ms + lapsed, (size_t)(q->count - lapsed) * sizeof(q->taken_ms[0]));
        q->count -= lapsed;
    }
    if (per_hour < 1 || q->count >= per_hour) {
        if (retry_after_s) *retry_after_s = q->count ? (int)((q->taken_ms[0] + HOUR_MS - now_ms + 999) / 1000) : 3600;
        return 0;
    }
    q->taken_ms[q->count++] = now_ms;
    return 1;
}
