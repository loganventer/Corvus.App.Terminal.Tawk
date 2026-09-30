#ifndef APP_UTILITIES_CLOCK_UTIL_H
#define APP_UTILITIES_CLOCK_UTIL_H

#include <stddef.h>
#include <stdint.h>

/* Monotonic milliseconds, for timers and idle detection. */
int64_t clock_now_ms(void);
/* "14:05" or "2:05 PM" */
void    clock_format_time(int64_t epoch_s, int use_24h, char *out, size_t size);
/* "14:05" today, "Yesterday", weekday within a week, else "29/09/2026". */
void    clock_format_relative(int64_t epoch_s, int use_24h, char *out, size_t size);
/* Terse form for narrow columns: "14:05" today, "y" yesterday, "mo".."su"
 * within a week, else "29/09" (this year) or "29/09/25". */
void    clock_format_short(int64_t epoch_s, int use_24h, char *out, size_t size);
/* "Today", "Yesterday", "Monday", or "29 September 2026". */
void    clock_format_day(int64_t epoch_s, char *out, size_t size);
/* A time still to come: "Today 18:00", "Tomorrow 09:00", "Friday 17:30"
 * within a week, else "12 Oct 18:00". */
void    clock_format_upcoming(int64_t epoch_s, int use_24h, char *out, size_t size);
/* Days since epoch in local time, used to detect day boundaries. */
int64_t clock_local_day(int64_t epoch_s);

#endif
