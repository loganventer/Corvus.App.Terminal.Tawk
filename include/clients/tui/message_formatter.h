#ifndef APP_CLIENTS_TUI_MESSAGE_FORMATTER_H
#define APP_CLIENTS_TUI_MESSAGE_FORMATTER_H

#include <stddef.h>

#include "core/message.h"
#include "core/styled_text.h"

/* How the views get a message's text as it is shown, without knowing the
 * formatting rules or who a mention names. */
typedef struct MessageFormatter {
    void *ctx;
    /* Fills `out` (the caller disposes it); returns -1 when the raw text is
     * to be shown as it is (formatting turned off, no text). */
    int  (*format)(void *ctx, const Message *message, StyledText *out);
    /* One line for lists and search results. */
    void (*preview)(void *ctx, const Message *message, char *out, size_t size);
} MessageFormatter;

#endif
