#include "engines/schedule_time_parser.h"

#include <ctype.h>
#include <string.h>
#include <strings.h>
#include <time.h>

#define DEFAULT_HOUR 9

static const char *skip_spaces(const char *p) {
    while (*p == ' ' || *p == '\t') p++;
    return p;
}

/* The length of the word at p (letters only). */
static size_t word_length(const char *p) {
    size_t n = 0;
    while (isalpha((unsigned char)p[n])) n++;
    return n;
}

static int word_is(const char *p, size_t n, const char *word) {
    return n == strlen(word) && strncasecmp(p, word, n) == 0;
}

/* "+1h30m": minutes, hours and days from now. */
static int parse_offset(const char *p, int64_t now, int64_t *due, const char **rest) {
    int64_t seconds = 0;
    int parts = 0;
    p++;                                               /* the '+' */
    while (isdigit((unsigned char)*p)) {
        long value = 0;
        while (isdigit((unsigned char)*p) && value < 100000) value = value * 10 + (*p++ - '0');
        char unit = (char)tolower((unsigned char)*p);
        if (unit == 'm') seconds += value * 60;
        else if (unit == 'h') seconds += value * 3600;
        else if (unit == 'd') seconds += value * 86400;
        else return -1;
        p++;
        parts++;
    }
    if (!parts || seconds <= 0 || (*p && *p != ' ' && *p != '\t')) return -1;
    *due = now + seconds;
    *rest = skip_spaces(p);
    return 0;
}

/* "18:00", "6:30pm", "7pm". Returns the characters used, 0 when there is no time. */
static int parse_clock(const char *p, int *hour, int *minute) {
    const char *s = p;
    int h = 0, m = 0, digits = 0;
    while (isdigit((unsigned char)*p) && digits < 2) { h = h * 10 + (*p++ - '0'); digits++; }
    if (!digits) return 0;
    int has_minutes = 0;
    if (*p == ':' || *p == '.') {
        p++;
        if (!isdigit((unsigned char)p[0]) || !isdigit((unsigned char)p[1])) return 0;
        m = (p[0] - '0') * 10 + (p[1] - '0');
        p += 2;
        has_minutes = 1;
    }
    if (strncasecmp(p, "am", 2) == 0 || strncasecmp(p, "pm", 2) == 0) {
        int pm = tolower((unsigned char)*p) == 'p';
        if (h < 1 || h > 12) return 0;
        h = h % 12 + (pm ? 12 : 0);
        p += 2;
    } else if (!has_minutes) {
        return 0;                                      /* a bare number is not a time */
    }
    if (h > 23 || m > 59 || (*p && *p != ' ' && *p != '\t')) return 0;
    *hour = h;
    *minute = m;
    return (int)(p - s);
}

/* The epoch time of `days` days after today's date at hour:minute, local time. */
static int64_t at(int64_t now, int days, int hour, int minute) {
    time_t t = (time_t)now;
    struct tm tm;
    localtime_r(&t, &tm);
    tm.tm_mday += days;
    tm.tm_hour = hour;
    tm.tm_min = minute;
    tm.tm_sec = 0;
    tm.tm_isdst = -1;
    return (int64_t)mktime(&tm);
}

static int weekday_of(const char *p, size_t n) {
    static const char *const FULL[7] = { "sunday", "monday", "tuesday", "wednesday", "thursday", "friday", "saturday" };
    for (int d = 0; d < 7; d++) {
        if (n >= 3 && n <= strlen(FULL[d]) && strncasecmp(p, FULL[d], n) == 0) return d;
    }
    return -1;
}

typedef enum { WHEN_CLOCK, WHEN_TODAY, WHEN_TOMORROW, WHEN_WEEKDAY } WhenKind;

int schedule_time_parse(const char *text, int64_t now, int64_t *due, const char **rest) {
    if (!text) return -1;
    const char *p = skip_spaces(text);
    if (*p == '+') return parse_offset(p, now, due, rest);

    int hour = DEFAULT_HOUR, minute = 0, days = 0;
    WhenKind kind = WHEN_CLOCK;
    size_t n = word_length(p);
    if (n) {
        time_t t = (time_t)now;
        struct tm today;
        localtime_r(&t, &today);
        int weekday;
        if (word_is(p, n, "today")) kind = WHEN_TODAY;
        else if (word_is(p, n, "tomorrow")) { kind = WHEN_TOMORROW; days = 1; }
        else if ((weekday = weekday_of(p, n)) >= 0) { kind = WHEN_WEEKDAY; days = (weekday - today.tm_wday + 7) % 7; }
        else return -1;
        p = skip_spaces(p + n);
        int used = parse_clock(p, &hour, &minute);
        if (used) p = skip_spaces(p + used);
        else if (kind == WHEN_TODAY) return -1;        /* "today" needs a time */
    } else {
        int used = parse_clock(p, &hour, &minute);
        if (!used) return -1;
        p = skip_spaces(p + used);
    }
    int64_t when = at(now, days, hour, minute);
    if (when <= now) {
        switch (kind) {
            case WHEN_CLOCK:   when = at(now, 1, hour, minute); break;          /* that time tomorrow */
            case WHEN_WEEKDAY: when = at(now, days + 7, hour, minute); break;   /* that day next week */
            default:           return -1;
        }
    }
    *due = when;
    *rest = p;
    return 0;
}

int schedule_time_parse_adjustment(const char *text, int64_t *seconds) {
    if (!text) return -1;
    const char *p = skip_spaces(text);
    int sign = *p == '-' ? -1 : *p == '+' ? 1 : 0;
    if (!sign) return -1;
    p++;
    int64_t value = 0;
    int digits = 0;
    while (isdigit((unsigned char)*p) && value <= 86400) { value = value * 10 + (*p++ - '0'); digits++; }
    if (!digits || value > 86400 || tolower((unsigned char)*p) != 's') return -1;
    p = skip_spaces(p + 1);
    if (*p) return -1;
    *seconds = sign * value;
    return 0;
}
