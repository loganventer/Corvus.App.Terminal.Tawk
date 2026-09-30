#ifndef APP_CORE_STYLED_RUN_H
#define APP_CORE_STYLED_RUN_H

#include <stddef.h>

/* Bytes [start, end) of a styled text drawn in one style (TextStyle bits). */
typedef struct StyledRun {
    size_t start;
    size_t end;
    int    style;
} StyledRun;

#endif
