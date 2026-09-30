#ifndef APP_CLIENTS_TUI_TUI_DRAW_H
#define APP_CLIENTS_TUI_TUI_DRAW_H

#include "clients/tui/ui_rect.h"

/* Drawing helpers on stdscr. Text is UTF-8 and clipped by display width,
 * so wide glyphs and emoji never overflow a region. */
void tui_fill(UiRect rect, int attr);
/* Draws at most max_cols columns; returns the columns used. */
int  tui_text(int y, int x, int max_cols, const char *text, int attr);
/* Draws `len` bytes of text (not NUL-terminated). */
int  tui_text_n(int y, int x, int max_cols, const char *text, unsigned long len, int attr);
/* Right-aligns text so it ends at column right_x (exclusive). */
int  tui_text_right(int y, int right_x, int max_cols, const char *text, int attr);
/* Centres text within [x, x + w). */
void tui_text_center(int y, int x, int w, const char *text, int attr);
/* A vertical rule. */
void tui_vline(int y, int x, int n, int attr);
/* A rounded box outline with an optional title. */
void tui_box(UiRect rect, const char *title, int attr);

#endif
