#ifndef APP_INFRASTRUCTURE_OSC52_CLIPBOARD_H
#define APP_INFRASTRUCTURE_OSC52_CLIPBOARD_H

#include "contracts/i_clipboard.h"

/* Copies through the terminal with the OSC 52 escape, which Windows
 * Terminal, iTerm2, kitty, WezTerm, foot and tmux (set-clipboard on)
 * forward to the system clipboard. No helper programs are needed, and it
 * works over SSH too. */
IClipboard *osc52_clipboard_create(void);

#endif
