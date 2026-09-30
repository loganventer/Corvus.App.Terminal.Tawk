#ifndef APP_UTILITIES_STR_UTIL_H
#define APP_UTILITIES_STR_UTIL_H

#include <stddef.h>

/* Bounded copy that always terminates dst. Returns strlen(src). */
size_t str_copy(char *dst, size_t dst_size, const char *src);
/* Duplicates src; NULL stays NULL. */
char  *str_dup(const char *src);
/* Trims ASCII whitespace in place and returns the trimmed start. */
char  *str_trim(char *s);
int    str_starts_with(const char *s, const char *prefix);
/* Parses "true/false/yes/no/on/off/1/0". Returns fallback when unknown. */
int    str_parse_bool(const char *s, int fallback);
/* Parses a base-10 int and clamps it to [min, max]. Returns fallback when invalid. */
int    str_parse_int(const char *s, int min, int max, int fallback);
/* Replaces control characters (except none) with spaces so remote text can
 * never inject terminal escape sequences. Operates in place. */
void   str_strip_controls(char *s);

#endif
