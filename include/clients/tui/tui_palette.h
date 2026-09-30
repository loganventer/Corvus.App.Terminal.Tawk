#ifndef APP_CLIENTS_TUI_TUI_PALETTE_H
#define APP_CLIENTS_TUI_TUI_PALETTE_H

#include <ncurses.h>

#include "core/theme.h"

/* curses attributes as int, so they combine with colour pairs without sign warnings. */
#define ATTR_BOLD      ((int)A_BOLD)
#define ATTR_DIM       ((int)A_DIM)
#define ATTR_UNDERLINE ((int)A_UNDERLINE)
#define ATTR_REVERSE   ((int)A_REVERSE)
#define ATTR_NORMAL    ((int)A_NORMAL)

/* Maps theme slots onto curses colour pairs. Applying a new theme recolours
 * everything on the next refresh, which is how live preview works. */
void tui_palette_init(void);
void tui_palette_apply(const Theme *theme);
/* Colour-pair attribute for a slot. */
int  tui_palette_attr(ThemeSlot slot);
/* The conversation pane has its own pairs so a chat can have its own theme
 * while the rest of the screen keeps the app theme. */
void tui_palette_apply_conversation(const Theme *theme);
int  tui_palette_conversation_attr(ThemeSlot slot);
/* Bubble colour on the chat background, for the half-block edges that make
 * a bubble a solid rectangle. */
int  tui_palette_bubble_edge_attr(int from_me);
/* The accent colour on a bubble, for code and mentions inside a message. */
int  tui_palette_bubble_accent_attr(int from_me);

#endif
