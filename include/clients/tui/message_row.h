#ifndef APP_CLIENTS_TUI_MESSAGE_ROW_H
#define APP_CLIENTS_TUI_MESSAGE_ROW_H

#include <stddef.h>

#include "clients/tui/message_row_kind.h"

/* One laid-out screen row of the conversation. */
typedef struct MessageRow {
    int            message;   /* index into the message array */
    MessageRowKind kind;
    int            x;         /* bubble left edge, relative to the view */
    int            width;     /* bubble width */
    size_t         offset;    /* TEXT rows: byte range into the message text */
    size_t         length;
    int            meta_inline;  /* time and ticks share this row (last content row) */
    int            sub;          /* THUMB rows: which row of the preview */
} MessageRow;

#endif
