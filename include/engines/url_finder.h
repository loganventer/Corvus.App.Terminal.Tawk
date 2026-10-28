#ifndef APP_ENGINES_URL_FINDER_H
#define APP_ENGINES_URL_FINDER_H

#include <stddef.h>

/* Finds the first http:// or https:// address in `text` at or after byte `from`, without trailing
 * punctuation. *begin and *end receive its byte range. Returns 0 when one was found, -1 otherwise. */
int url_find_next(const char *text, size_t from, size_t *begin, size_t *end);

/* Copies the first http:// or https:// address in `text` to `out`, without
 * trailing punctuation. Returns 0 when one was found, -1 otherwise. */
int url_find_first(const char *text, char *out, size_t out_size);

#endif
