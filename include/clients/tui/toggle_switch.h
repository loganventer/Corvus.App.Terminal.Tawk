#ifndef APP_CLIENTS_TUI_TOGGLE_SWITCH_H
#define APP_CLIENTS_TUI_TOGGLE_SWITCH_H

#define TOGGLE_SWITCH_COLUMNS 3

/* A switch three columns wide: the knob on the right and lit when on, on
 * the left and dim when off. */
const char *toggle_switch_text(int on);
/* Draws it at (y, x) over `attr`'s background. */
void        toggle_switch_draw(int y, int x, int on, int attr);

#endif
