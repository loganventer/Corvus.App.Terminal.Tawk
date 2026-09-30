#ifndef APP_ENGINES_SCHEDULE_TIME_PARSER_H
#define APP_ENGINES_SCHEDULE_TIME_PARSER_H

#include <stdint.h>

/* Reads when a message should go from the start of `text`, in local time:
 *   18:00, 6:30pm      today, or tomorrow when that time has passed
 *   +30m, +2h, +1h30m, +1d   from now
 *   tomorrow 9:00      (9:00 when no time is given)
 *   today 18:00
 *   fri 17:30, monday  the next such day (today when the time is still ahead)
 * Sets *due (epoch seconds) and *rest to the text after the time, and
 * returns 0; returns -1 when there is no time there or it is not in the future. */
int schedule_time_parse(const char *text, int64_t now, int64_t *due, const char **rest);

#endif
