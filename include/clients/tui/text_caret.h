#ifndef APP_CLIENTS_TUI_TEXT_CARET_H
#define APP_CLIENTS_TUI_TEXT_CARET_H

/* Where a text field wants the (blinking) terminal cursor after it is
 * drawn. Fields set it while they take typing and clear it otherwise. */
typedef struct TextCaret {
    int visible;
    int y, x;
} TextCaret;

#endif
