#ifndef APP_CLIENTS_TUI_TYPING_INDICATOR_H
#define APP_CLIENTS_TUI_TYPING_INDICATOR_H

/* A small bubble at the bottom of the conversation saying who is typing or
 * recording, with three dots that light up in turn like WhatsApp's.
 * `text` may end in "…", which the dots replace. `phase` advances the dots. */
void typing_indicator_draw(int y, int x, int width, const char *text, int phase);

#endif
