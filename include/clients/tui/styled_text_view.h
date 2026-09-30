#ifndef APP_CLIENTS_TUI_STYLED_TEXT_VIEW_H
#define APP_CLIENTS_TUI_STYLED_TEXT_VIEW_H

#include <stddef.h>

#include "core/styled_text.h"

/* Draws bytes [from, from + length) of formatted text at (y, x), at most
 * max_cols wide, one call per differently styled part. `base` is the
 * attribute of plain text; `accent` the one for code and mentions.
 * Returns the columns used. */
int styled_text_view_draw(int y, int x, int max_cols, const StyledText *text, size_t from, size_t length, int base, int accent);

#endif
