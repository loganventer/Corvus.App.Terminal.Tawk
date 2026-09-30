#ifndef APP_INFRASTRUCTURE_OSC_TERMINAL_TITLE_H
#define APP_INFRASTRUCTURE_OSC_TERMINAL_TITLE_H

#include "contracts/i_terminal_title.h"

/* Sets the window title with the OSC 0 escape (xterm, Windows Terminal,
 * iTerm2, kitty, GNOME Terminal). Saves the previous title on create and
 * restores it on destroy where the terminal supports the title stack. */
ITerminalTitle *osc_terminal_title_create(void);

#endif
