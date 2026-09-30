#ifndef APP_UTILITIES_TEXT_LINE_H
#define APP_UTILITIES_TEXT_LINE_H

#include <stddef.h>

/* A wrapped line: a byte range into the source text and its display width. */
typedef struct TextLine {
    size_t offset;
    size_t length;
    int    columns;
} TextLine;

#endif
