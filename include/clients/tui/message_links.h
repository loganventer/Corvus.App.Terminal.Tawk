#ifndef APP_CLIENTS_TUI_MESSAGE_LINKS_H
#define APP_CLIENTS_TUI_MESSAGE_LINKS_H

#include "clients/tui/message_url_span.h"

/* The web addresses on one row of a message's text: the row shows bytes [offset, offset + length) of
 * `text`, starting at column `x` of the screen, in `room` columns. Each address that shows on the row
 * becomes a span (y is left for the caller). Returns how many were written, at most `max`. */
int message_links_in_text_row(const char *text, int offset, int length, int x, int room, MessageUrlSpan *out, int max);

/* A link card's title or site line opens the card's address: one span over `label` at column `x`. */
int message_links_on_card_row(const char *url, const char *label, int x, int room, MessageUrlSpan *out);

#endif
