#ifndef APP_UTILITIES_UTF8_TEXT_H
#define APP_UTILITIES_UTF8_TEXT_H

#include <stddef.h>

#include "utilities/text_line.h"

/* Display columns of a UTF-8 string (wide glyphs count as two). */
int    utf8_columns(const char *s);
/* Bytes of `s` that fit within max_columns; *columns receives the width used. */
size_t utf8_fit(const char *s, size_t length, int max_columns, int *columns);
/* Word-wraps text to `width` columns, honouring newlines. Returns the line
 * count and a malloc'd array the caller frees. */
int    utf8_wrap(const char *text, int width, TextLine **lines);
/* Converts a wide string to UTF-8. Returns a malloc'd string. */
char  *utf8_from_wide(const wchar_t *ws, size_t count);

#endif
