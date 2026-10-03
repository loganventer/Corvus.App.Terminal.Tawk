#ifndef APP_ENGINES_HOURLY_QUOTA_H
#define APP_ENGINES_HOURLY_QUOTA_H

#include <stdint.h>

#define HOURLY_QUOTA_MAX 240

/* At most so many in any hour: remembers when each of the last ones was taken. */
typedef struct HourlyQuota {
    int64_t taken_ms[HOURLY_QUOTA_MAX];     /* oldest first */
    int     count;
} HourlyQuota;

void hourly_quota_init(HourlyQuota *quota);
/* Takes one; returns 1 when `per_hour` was not yet reached in the last hour,
 * else 0 with the seconds until the oldest one lapses in *retry_after_s. */
int  hourly_quota_take(HourlyQuota *quota, int per_hour, int64_t now_ms, int *retry_after_s);

#endif
