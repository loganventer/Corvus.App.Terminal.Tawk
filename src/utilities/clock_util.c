#include "utilities/clock_util.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

int64_t clock_now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

static struct tm local_tm(int64_t epoch_s) {
    time_t t = (time_t)epoch_s;
    struct tm out;
    localtime_r(&t, &out);
    return out;
}

int64_t clock_local_day(int64_t epoch_s) {
    struct tm tm_v = local_tm(epoch_s);
    return (int64_t)(tm_v.tm_year) * 400 + tm_v.tm_yday;
}

void clock_format_time(int64_t epoch_s, int use_24h, char *out, size_t size) {
    struct tm tm_v = local_tm(epoch_s);
    strftime(out, size, use_24h ? "%H:%M" : "%I:%M %p", &tm_v);
    if (out[0] == '0') memmove(out, out + 1, strlen(out));
}

void clock_format_relative(int64_t epoch_s, int use_24h, char *out, size_t size) {
    int64_t now = (int64_t)time(NULL);
    int64_t diff_days = clock_local_day(now) - clock_local_day(epoch_s);
    struct tm tm_v = local_tm(epoch_s);
    if (epoch_s <= 0) { out[0] = '\0'; return; }
    if (diff_days == 0) clock_format_time(epoch_s, use_24h, out, size);
    else if (diff_days == 1) snprintf(out, size, "Yesterday");
    else if (diff_days > 1 && diff_days < 7) strftime(out, size, "%A", &tm_v);
    else strftime(out, size, "%d/%m/%Y", &tm_v);
}

void clock_format_short(int64_t epoch_s, int use_24h, char *out, size_t size) {
    static const char *const DAYS[7] = { "su", "mo", "tu", "we", "th", "fr", "sa" };
    if (epoch_s <= 0) { out[0] = '\0'; return; }
    int64_t now = (int64_t)time(NULL);
    int64_t diff_days = clock_local_day(now) - clock_local_day(epoch_s);
    struct tm tm_v = local_tm(epoch_s), tm_now = local_tm(now);
    if (diff_days == 0) clock_format_time(epoch_s, use_24h, out, size);
    else if (diff_days == 1) snprintf(out, size, "y");
    else if (diff_days > 1 && diff_days < 7) snprintf(out, size, "%s", DAYS[tm_v.tm_wday]);
    else if (tm_v.tm_year == tm_now.tm_year) strftime(out, size, "%d/%m", &tm_v);
    else snprintf(out, size, "%02d/%02d/%02d", tm_v.tm_mday, tm_v.tm_mon + 1, tm_v.tm_year % 100);
}

void clock_format_day(int64_t epoch_s, char *out, size_t size) {
    int64_t now = (int64_t)time(NULL);
    int64_t diff_days = clock_local_day(now) - clock_local_day(epoch_s);
    struct tm tm_v = local_tm(epoch_s);
    if (diff_days == 0) snprintf(out, size, "Today");
    else if (diff_days == 1) snprintf(out, size, "Yesterday");
    else if (diff_days > 1 && diff_days < 7) strftime(out, size, "%A", &tm_v);
    else strftime(out, size, "%d %B %Y", &tm_v);
}

/* Local midnight of the day holding epoch_s. */
static int64_t midnight(int64_t epoch_s) {
    struct tm tm_v = local_tm(epoch_s);
    tm_v.tm_hour = tm_v.tm_min = tm_v.tm_sec = 0;
    tm_v.tm_isdst = -1;
    return (int64_t)mktime(&tm_v);
}

void clock_format_upcoming(int64_t epoch_s, int use_24h, char *out, size_t size) {
    char time_part[24];
    clock_format_time(epoch_s, use_24h, time_part, sizeof(time_part));
    int64_t days = (midnight(epoch_s) - midnight((int64_t)time(NULL)) + 43200) / 86400;   /* rounded: DST days are 23 or 25 hours */
    struct tm tm_v = local_tm(epoch_s);
    char day[32];
    if (days <= 0) snprintf(day, sizeof(day), "Today");
    else if (days == 1) snprintf(day, sizeof(day), "Tomorrow");
    else if (days < 7) strftime(day, sizeof(day), "%A", &tm_v);
    else strftime(day, sizeof(day), "%d %b", &tm_v);
    snprintf(out, size, "%s %s", day, time_part);
}
