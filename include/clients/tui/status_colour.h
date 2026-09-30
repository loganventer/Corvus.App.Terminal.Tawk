#ifndef APP_CLIENTS_TUI_STATUS_COLOUR_H
#define APP_CLIENTS_TUI_STATUS_COLOUR_H

#include <stdint.h>

/* The curses attribute for white text on a text status's background
 * colour (ARGB), or 0 when the terminal cannot show it. */
int status_colour_attr(uint32_t argb);

#endif
