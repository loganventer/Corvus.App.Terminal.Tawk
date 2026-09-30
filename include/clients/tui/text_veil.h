#ifndef APP_CLIENTS_TUI_TEXT_VEIL_H
#define APP_CLIENTS_TUI_TEXT_VEIL_H

/* Draws a soft shaded band `cols` wide in place of text or a picture: the
 * shape of the content stays, the content does not (a soft-locked chat). */
void text_veil_draw(int y, int x, int cols, int attr);
/* Draws the veil over the columns a UTF-8 text (of `length` bytes) would use. */
void text_veil_text(int y, int x, int max_cols, const char *text, unsigned long length, int attr);

#endif
